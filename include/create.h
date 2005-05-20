/** \file
 */

#ifndef OPS_CREATE_H
#define OPS_CREATE_H

#include "types.h"
#include "packet.h"
#include "crypto.h"
#include "memory.h"

enum ops_writer_ret_t
    {
    OPS_W_OK		=0,
    OPS_W_ERROR		=1,
    };

typedef ops_writer_ret_t ops_packet_writer_t(const unsigned char *src,
					     unsigned length,
					     ops_writer_flags_t flags,
					     void *arg);

typedef struct
    {
    ops_packet_writer_t *writer;
    void *arg;
    } ops_create_options_t;

typedef struct
    {
    ops_packet_writer_t *writer;
    void *arg;
    ops_hash_t hash;
    ops_signature_t sig;
    ops_memory_t mem;
    ops_create_options_t opt;
    unsigned hashed_count_offset;
    unsigned hashed_data_length;
    unsigned unhashed_count_offset;
    } ops_create_signature_t;

ops_boolean_t ops_write(const void *src,unsigned length,
			ops_create_options_t *opt);
ops_boolean_t ops_write_length(unsigned length,ops_create_options_t *opt);
ops_boolean_t ops_write_ptag(ops_content_tag_t tag,ops_create_options_t *opt);
ops_boolean_t ops_write_scalar(unsigned n,unsigned length,
			       ops_create_options_t *opt);

void ops_fast_create_rsa_public_key(ops_public_key_t *key,time_t time,
				    BIGNUM *n,BIGNUM *e);
void ops_create_rsa_public_key(ops_public_key_t *key,time_t time,
			       const BIGNUM *n,const BIGNUM *e);
ops_boolean_t ops_write_struct_public_key(const ops_public_key_t *key,
					  ops_create_options_t *opt);
ops_boolean_t ops_write_rsa_public_key(time_t time,const BIGNUM *n,
				       const BIGNUM *e,
				       ops_create_options_t *opt);

void ops_fast_create_user_id(ops_user_id_t *id,char *user_id);
ops_boolean_t ops_write_struct_user_id(ops_user_id_t *id,
				       ops_create_options_t *opt);
ops_boolean_t ops_write_user_id(const char *user_id,ops_create_options_t *opt);

#endif