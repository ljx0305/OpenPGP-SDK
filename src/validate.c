#include <openpgpsdk/packet-parse.h>
#include <openpgpsdk/keyring.h>
#include "keyring_local.h"
#include <openpgpsdk/util.h>
#include <openpgpsdk/signature.h>
#include <assert.h>
#include <string.h>

/* XXX: because we might well use this reader with different callbacks
   it would make sense to split the arguments for callbacks, one for the reader
   and one for the callback */
typedef struct
    {
    const ops_key_data_t *key;
    unsigned packet;
    unsigned offset;
    } validate_reader_arg_t;

typedef struct
    {
    ops_public_key_t pkey;
    ops_public_key_t subkey;
    ops_user_id_t user_id;
    const ops_keyring_t *keyring;
    validate_reader_arg_t *rarg;
    } validate_cb_arg_t;

static ops_reader_ret_t key_data_reader(unsigned char *dest,unsigned *plength,
					ops_reader_flags_t flags,void *arg_)
    {
    validate_reader_arg_t *arg=arg_;

    OPS_USED(flags);
    if(arg->offset == arg->key->packets[arg->packet].length)
	{
	++arg->packet;
	arg->offset=0;
	}

    if(arg->packet == arg->key->npackets)
	return OPS_R_EOF;

    // we should never be asked to cross a packet boundary in a single read
    assert(arg->key->packets[arg->packet].length >= arg->offset+*plength);

    memcpy(dest,&arg->key->packets[arg->packet].raw[arg->offset],*plength);
    arg->offset+=*plength;

     return OPS_R_OK;
     }

/**
 * \ingroup Callbacks
 */

static ops_parse_callback_return_t
validate_cb(const ops_parser_content_t *content_,void *arg_)
     {
     const ops_parser_content_union_t *content=&content_->content;
     validate_cb_arg_t *arg=arg_;
     const ops_key_data_t *signer;
     ops_boolean_t valid;

     switch(content_->tag)
	 {
     case OPS_PTAG_CT_PUBLIC_KEY:
	 assert(arg->pkey.version == 0);
	 arg->pkey=content->public_key;
	 return OPS_KEEP_MEMORY;

     case OPS_PTAG_CT_PUBLIC_SUBKEY:
	 if(arg->subkey.version)
	     ops_public_key_free(&arg->subkey);
	 arg->subkey=content->public_key;
	 return OPS_KEEP_MEMORY;

     case OPS_PTAG_CT_USER_ID:
	 printf("user id=%s\n",content->user_id.user_id);
	 if(arg->user_id.user_id)
	     ops_user_id_free(&arg->user_id);
	 arg->user_id=content->user_id;
	 return OPS_KEEP_MEMORY;

     case OPS_PTAG_CT_SIGNATURE:
	 printf("  type=%02x signer_id=",content->signature.type);
	 hexdump(content->signature.signer_id,
		 sizeof content->signature.signer_id);

	 signer=ops_keyring_find_key_by_id(arg->keyring,
					   content->signature.signer_id);
	 if(!signer)
	     {
	     printf(" UNKNOWN SIGNER\n");
	     break;
	     }

	 switch(content->signature.type)
	     {
	 case OPS_CERT_GENERIC:
	 case OPS_CERT_PERSONA:
	 case OPS_CERT_CASUAL:
	 case OPS_CERT_POSITIVE:
	 case OPS_SIG_REV_CERT:
	     valid=ops_check_certification_signature(&arg->pkey,&arg->user_id,
		     &content->signature,&signer->pkey,
		     arg->rarg->key->packets[arg->rarg->packet].raw);
	     break;

	 case OPS_SIG_SUBKEY:
	     // XXX: we should also check that the signer is the key we are validating, I think.
	     valid=ops_check_subkey_signature(&arg->pkey,&arg->subkey,
		     &content->signature,&signer->pkey,
		     arg->rarg->key->packets[arg->rarg->packet].raw);
	     break;

	 default:
	     fprintf(stderr,"Unexpected signature type=0x%02x\n",
		     content->signature.type);
	     exit(1);
	     }
	 if(valid)
	     printf(" validated\n");
	 else
	     printf(" BAD SIGNATURE\n");
	 break;

     default:
	 // XXX: reinstate when we can make packets optional
	 //	fprintf(stderr,"unexpected tag=%d\n",content_->tag);
	 break;
	 }
     return OPS_RELEASE_MEMORY;
     }

static void validate_key_signatures(const ops_key_data_t *key,
				    const ops_keyring_t *keyring)
     {
     ops_parse_options_t opt;
     validate_cb_arg_t carg;
     validate_reader_arg_t rarg;

     memset(&rarg,'\0',sizeof rarg);
     memset(&carg,'\0',sizeof carg);

     ops_parse_options_init(&opt);
     //    ops_parse_options(&opt,OPS_PTAG_CT_SIGNATURE,OPS_PARSE_PARSED);
     opt.cb=validate_cb;
    opt.reader=key_data_reader;

    rarg.key=key;
    rarg.packet=0;
    rarg.offset=0;

    carg.keyring=keyring;
    carg.rarg=&rarg;

    opt.cb_arg=&carg;
    opt.reader_arg=&rarg;

    ops_parse(&opt);

    ops_public_key_free(&carg.pkey);
    if(carg.subkey.version)
	ops_public_key_free(&carg.subkey);
    ops_user_id_free(&carg.user_id);
    }

void ops_validate_all_signatures(const ops_keyring_t *ring)
    {
    int n;

    for(n=0 ; n < ring->nkeys ; ++n)
	validate_key_signatures(&ring->keys[n],ring);
    }
