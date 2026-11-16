#include "targets/idis_ip_meter/cosem/classes/nd_ic_association.h"

#include <string.h>

#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

void nd_idis_association_init(nd_idis_association_t *state)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->association.client_sap = 16;
    state->association.status = 1;
    (void)nd_idis_registry_add(&state->association.base, 8, 0,
                               &ND_OBIS_CLOCK);
    (void)nd_idis_registry_add(&state->association.base, 3, 0,
                               &ND_OBIS_ACTIVE_ENERGY_IMPORT);
    (void)nd_idis_registry_add(&state->association.base, 15, 0,
                               &ND_OBIS_ASSOCIATION);
    (void)nd_idis_registry_add(&state->association.base, 64, 0,
                               &ND_OBIS_SECURITY_SETUP);
    (void)nd_idis_registry_add(&state->association.base, 17, 0,
                               &ND_OBIS_SAP_ASSIGNMENT);
    state->sap_count = 1;
    state->sap_list[0].sap = 16;
    state->sap_list[0].ln = ND_OBIS_ASSOCIATION;
    state->security.policy = 0;
    state->security.suite = 0;
    state->consumer = state->security;
}

void nd_idis_association_bind_crypto(nd_idis_association_t *state,
                                     const nd_crypto_port_t *crypto)
{
    if (state != NULL) {
        state->crypto = crypto;
    }
}

static int put_ln(nd_axdr_writer_t *out, const nd_obis_t *ln)
{
    nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(out, 6);
    nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
    return 0;
}

static void put_access_list(nd_axdr_writer_t *out,
                            const nd_assoc_access_t *items, uint8_t count,
                            int with_selector)
{
    nd_axdr_put_array_header(out, count);
    for (uint8_t index = 0; index < count; index++) {
        nd_axdr_put_structure_header(out, with_selector ? 3 : 2);
        nd_axdr_put_tag(out, ND_AXDR_INTEGER);
        nd_axdr_put_u8(out, items[index].id);
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, items[index].access);
        if (with_selector) {
            nd_axdr_put_tag(out, ND_AXDR_NULL);
        }
    }
}

static int read_typed_u8(nd_axdr_reader_t *in, uint8_t tag, uint8_t *value)
{
    if (nd_axdr_get_tag(in) != tag) {
        return -1;
    }
    *value = nd_axdr_get_u8(in);
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int read_long_unsigned(nd_axdr_reader_t *in, uint16_t *value)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_LONG_UNSIGNED) {
        return -1;
    }
    *value = nd_axdr_get_u16(in);
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int read_octets(nd_axdr_reader_t *in, uint8_t *out, uint8_t capacity,
                       uint8_t *length)
{
    uint8_t tag = nd_axdr_get_tag(in);
    if (tag != ND_AXDR_OCTET_STRING && tag != ND_AXDR_VISIBLE_STRING) {
        return -1;
    }
    uint16_t size = nd_axdr_get_length(in);
    if (size > capacity) {
        return -1;
    }
    nd_axdr_get_octets(in, out, size);
    if (!nd_axdr_reader_ok(in)) {
        return -1;
    }
    if (length != NULL) {
        *length = (uint8_t)size;
    }
    return 0;
}

static int read_access_list(nd_axdr_reader_t *in, nd_assoc_access_t *items,
                            uint8_t capacity, uint8_t *count,
                            int with_selector)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
        return -1;
    }
    uint16_t total = nd_axdr_get_length(in);
    if (total > capacity) {
        return -1;
    }
    for (uint16_t index = 0; index < total; index++) {
        if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
            || nd_axdr_get_length(in) < (with_selector ? 3 : 2)
            || nd_axdr_get_tag(in) != ND_AXDR_INTEGER) {
            return -1;
        }
        items[index].id = nd_axdr_get_u8(in);
        if (nd_axdr_get_tag(in) != ND_AXDR_ENUM) {
            return -1;
        }
        items[index].access = nd_axdr_get_u8(in);
        if (with_selector && nd_axdr_skip_value(in) != 0) {
            return -1;
        }
    }
    *count = (uint8_t)total;
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int read_object_entry(nd_axdr_reader_t *in, nd_assoc_object_t *entry)
{
    memset(entry, 0, sizeof(*entry));
    if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
        || nd_axdr_get_length(in) < 4
        || nd_axdr_get_tag(in) != ND_AXDR_LONG_UNSIGNED) {
        return -1;
    }
    entry->class_id = nd_axdr_get_u16(in);
    if (nd_axdr_get_tag(in) != ND_AXDR_UNSIGNED) {
        return -1;
    }
    entry->version = nd_axdr_get_u8(in);
    uint8_t ln[6];
    uint8_t length = 0;
    if (read_octets(in, ln, sizeof(ln), &length) != 0 || length != 6) {
        return -1;
    }
    memcpy(&entry->ln, ln, 6);
    if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
        || nd_axdr_get_length(in) < 2) {
        return -1;
    }
    if (read_access_list(in, entry->attributes, ND_ASSOC_ACCESS_MAX,
                         &entry->attribute_count, 1) != 0
        || read_access_list(in, entry->methods, ND_ASSOC_ACCESS_MAX,
                            &entry->method_count, 0) != 0) {
        return -1;
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static nd_assoc_access_t access_for_class(uint8_t class_id)
{
    static const struct {
        uint8_t class_id;
        nd_assoc_access_t access;
    } policy[] = {
        {8, {2, 3}},  /* clock: value read and write */
        {3, {2, 1}},  /* register: value read */
        {4, {2, 1}},  /* extended register: value read */
    };
    for (size_t index = 0; index < sizeof(policy) / sizeof(policy[0]); index++) {
        if (policy[index].class_id == class_id) {
            return policy[index].access;
        }
    }
    return (nd_assoc_access_t){1, 1}; /* logical name read */
}

static int assoc_get(nd_assoc_state_t *state, const nd_obis_t *ln,
                     uint8_t attribute_id, nd_axdr_writer_t *out)
{
    switch (attribute_id) {
    case 1:
        put_ln(out, ln);
        break;
    case 2: {
        uint16_t total = (uint16_t)(state->base.count + state->extra_count);
        nd_axdr_put_array_header(out, total);
        for (uint16_t index = 0; index < state->base.count; index++) {
            const nd_idis_registry_entry_t *entry = &state->base.entries[index];
            nd_assoc_access_t access = access_for_class(entry->class_id);
            nd_axdr_put_structure_header(out, 4);
            nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
            nd_axdr_put_u16(out, entry->class_id);
            nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
            nd_axdr_put_u8(out, entry->version);
            put_ln(out, &entry->ln);
            nd_axdr_put_structure_header(out, 2);
            put_access_list(out, &access, 1, 1);
            put_access_list(out, NULL, 0, 0);
        }
        for (uint8_t index = 0; index < state->extra_count; index++) {
            const nd_assoc_object_t *entry = &state->extra[index];
            nd_axdr_put_structure_header(out, 4);
            nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
            nd_axdr_put_u16(out, entry->class_id);
            nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
            nd_axdr_put_u8(out, entry->version);
            put_ln(out, &entry->ln);
            nd_axdr_put_structure_header(out, 2);
            put_access_list(out, entry->attributes, entry->attribute_count, 1);
            put_access_list(out, entry->methods, entry->method_count, 0);
        }
        break;
    }
    case 3:
        nd_axdr_put_structure_header(out, 2);
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, state->client_sap);
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, state->server_sap);
        break;
    case 4:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, state->app_context_len);
        nd_axdr_put_octets(out, state->app_context, state->app_context_len);
        break;
    case 5: {
        static const uint8_t conformance[3] = {0, 0, 0};
        nd_axdr_put_structure_header(out, 6);
        nd_axdr_put_tag(out, ND_AXDR_BIT_STRING);
        nd_axdr_put_length(out, 24);
        nd_axdr_put_octets(out, conformance, 3);
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, 0x007F); /* max receive PDU */
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, 0x007F); /* max send PDU */
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, 6); /* DLMS version */
        nd_axdr_put_tag(out, ND_AXDR_INTEGER);
        nd_axdr_put_u8(out, 0xFF); /* quality of service: not used */
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 0); /* ciphered APDU: not used */
        break;
    }
    case 6:
        nd_axdr_put_structure_header(out, 2);
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, 0); /* mechanism */
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, 0); /* status */
        break;
    case 7:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, state->secret_len);
        nd_axdr_put_octets(out, state->secret, state->secret_len);
        break;
    case 8:
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, state->status);
        break;
    case 9:
        put_ln(out, &state->security_reference);
        break;
    case 10:
        nd_axdr_put_array_header(out, state->user_count);
        for (uint8_t user = 0; user < state->user_count; user++) {
            nd_axdr_put_structure_header(out, 2);
            nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
            nd_axdr_put_u8(out, user + 1);
            nd_axdr_put_tag(out, ND_AXDR_VISIBLE_STRING);
            nd_axdr_put_length(out, state->user_len[user]);
            nd_axdr_put_octets(out, state->users[user], state->user_len[user]);
        }
        break;
    case 11:
        nd_axdr_put_structure_header(out, 2);
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, state->user_count > 0 ? 1 : 0);
        nd_axdr_put_tag(out, ND_AXDR_VISIBLE_STRING);
        nd_axdr_put_length(out, state->user_count > 0 ? state->user_len[0] : 0);
        nd_axdr_put_octets(out, state->users[0],
                           state->user_count > 0 ? state->user_len[0] : 0);
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

static int security_get(nd_security_state_t *state, const nd_obis_t *ln,
                        uint8_t attribute_id, nd_axdr_writer_t *out)
{
    switch (attribute_id) {
    case 1:
        put_ln(out, ln);
        break;
    case 2:
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, state->policy);
        break;
    case 3:
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, state->suite);
        break;
    case 4:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 8);
        nd_axdr_put_octets(out, state->client_title, 8);
        break;
    case 5:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 8);
        nd_axdr_put_octets(out, state->server_title, 8);
        break;
    case 6:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, state->cert_len);
        nd_axdr_put_octets(out, state->cert, state->cert_len);
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_association_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                          uint8_t attribute_id,
                          const nd_axdr_reader_t *selector,
                          nd_axdr_writer_t *out)
{
    nd_idis_association_t *state = ctx;
    (void)client_sap;
    (void)selector;
    if (state == NULL || ln == NULL || out == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (nd_obis_eq(ln, &ND_OBIS_ASSOCIATION)) {
        return assoc_get(&state->association, ln, attribute_id, out);
    }
    if (nd_obis_eq(ln, &ND_OBIS_SAP_ASSIGNMENT)) {
        if (attribute_id == 1) {
            put_ln(out, ln);
        } else if (attribute_id == 2) {
            nd_axdr_put_array_header(out, state->sap_count);
            for (uint8_t index = 0; index < state->sap_count; index++) {
                nd_axdr_put_structure_header(out, 2);
                nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
                nd_axdr_put_u16(out, state->sap_list[index].sap);
                put_ln(out, &state->sap_list[index].ln);
            }
        } else {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
        return nd_axdr_writer_ok(out) ? ND_OK
                                      : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (nd_obis_eq(ln, &ND_OBIS_SECURITY_SETUP)) {
        return security_get(&state->security, ln, attribute_id, out);
    }
    if (nd_obis_eq(ln, &ND_OBIS_SECURITY_SETUP_CONSUMER)) {
        return security_get(&state->consumer, ln, attribute_id, out);
    }
    return ND_ERR_OBJECT_UNDEFINED;
}

int nd_ic_association_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                          uint8_t attribute_id, nd_axdr_reader_t *in)
{
    nd_idis_association_t *state = ctx;
    (void)client_sap;
    if (state == NULL || ln == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (nd_obis_eq(ln, &ND_OBIS_SAP_ASSIGNMENT)) {
        if (attribute_id != 2 || nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
            return ND_ERR_ATTRIBUTE_UNDEFINED;
        }
        uint16_t count = nd_axdr_get_length(in);
        if (count > ND_ASSOC_OBJECTS_MAX) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint8_t sap_count = 0;
        for (uint16_t index = 0; index < count; index++) {
            if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
                || nd_axdr_get_length(in) < 2) {
                return ND_ERR_TYPE_MISMATCH;
            }
            uint16_t sap = 0;
            if (read_long_unsigned(in, &sap) != 0 || sap > 0xFF) {
                return ND_ERR_TYPE_MISMATCH;
            }
            uint8_t body[6];
            uint8_t length = 0;
            if (read_octets(in, body, sizeof(body), &length) != 0
                || length != 6) {
                return ND_ERR_TYPE_MISMATCH;
            }
            state->sap_list[sap_count].sap = (uint8_t)sap;
            memcpy(&state->sap_list[sap_count].ln, body, 6);
            sap_count++;
        }
        state->sap_count = sap_count;
        return ND_OK;
    }
    nd_security_state_t *security = NULL;
    if (nd_obis_eq(ln, &ND_OBIS_SECURITY_SETUP)) {
        security = &state->security;
    } else if (nd_obis_eq(ln, &ND_OBIS_SECURITY_SETUP_CONSUMER)) {
        security = &state->consumer;
    } else {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 2 || attribute_id == 3) {
        uint8_t value = 0;
        if (read_typed_u8(in, ND_AXDR_ENUM, &value) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        if (attribute_id == 2) {
            security->policy = value;
        } else {
            security->suite = value;
        }
        return ND_OK;
    }
    if (attribute_id >= 4 && attribute_id <= 5) {
        uint8_t value[8];
        if (read_octets(in, value, sizeof(value), NULL) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        memcpy(attribute_id == 4 ? security->client_title
                                 : security->server_title,
               value, 8);
        return ND_OK;
    }
    if (attribute_id == 6) {
        return read_octets(in, security->cert, sizeof(security->cert),
                           &security->cert_len) == 0
                   ? ND_OK
                   : ND_ERR_TYPE_MISMATCH;
    }
    return ND_ERR_ATTRIBUTE_UNDEFINED;
}

static int assoc_action(nd_assoc_state_t *state, uint8_t method_id,
                        nd_axdr_reader_t *in, nd_axdr_writer_t *out,
                        const nd_crypto_port_t *crypto)
{
    if (method_id == 1) { /* reply_to_HLS_authentication */
        if (crypto == NULL || crypto->sha256 == NULL || out == NULL) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        uint8_t challenge[32];
        uint8_t length = 0;
        if (read_octets(in, challenge, sizeof(challenge), &length) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint8_t input[sizeof(challenge) + sizeof(state->secret)];
        memcpy(input, challenge, length);
        memcpy(input + length, state->secret, state->secret_len);
        uint8_t digest[32];
        if (crypto->sha256(crypto->ctx, input,
                           (size_t)length + state->secret_len, digest) != 0) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, sizeof(digest));
        nd_axdr_put_octets(out, digest, sizeof(digest));
        return nd_axdr_writer_ok(out) ? ND_OK
                                      : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (method_id == 2) { /* change_HLS_secret */
        return read_octets(in, state->secret, sizeof(state->secret),
                           &state->secret_len) == 0
                   ? ND_OK
                   : ND_ERR_TYPE_MISMATCH;
    }
    if (method_id == 3) { /* add_object */
        if (state->extra_count >= ND_ASSOC_OBJECTS_MAX) {
            return ND_ERR_TYPE_MISMATCH;
        }
        nd_assoc_object_t entry;
        if (read_object_entry(in, &entry) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->extra[state->extra_count++] = entry;
        return ND_OK;
    }
    if (method_id == 4) { /* remove_object */
        nd_assoc_object_t entry;
        if (read_object_entry(in, &entry) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        for (uint8_t index = 0; index < state->extra_count; index++) {
            if (state->extra[index].class_id == entry.class_id
                && memcmp(&state->extra[index].ln, &entry.ln, 6) == 0) {
                state->extra[index] = state->extra[state->extra_count - 1];
                state->extra_count--;
                return ND_OK;
            }
        }
        if (nd_idis_registry_remove(&state->base, (uint8_t)entry.class_id,
                                    &entry.ln) == 0) {
            return ND_OK;
        }
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (method_id == 5) { /* add_user */
        if (state->user_count >= ND_ASSOC_USERS_MAX) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        if (nd_axdr_get_tag(in) == ND_AXDR_STRUCTURE
            && nd_axdr_get_length(in) >= 2) {
            /* standard wraps {user id, user name}; the unwrapped form is
             * also accepted for the skeleton's script tables. */
        }
        uint8_t user_id = 0;
        if (read_typed_u8(in, ND_AXDR_UNSIGNED, &user_id) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        (void)user_id;
        return read_octets(in, state->users[state->user_count],
                           ND_ASSOC_USER_LEN,
                           &state->user_len[state->user_count]) == 0
                   ? (state->user_count++, ND_OK)
                   : ND_ERR_TYPE_MISMATCH;
    }
    if (method_id == 6) { /* remove_user */
        nd_axdr_reader_t probe = *in;
        if (nd_axdr_get_tag(&probe) == ND_AXDR_STRUCTURE
            && nd_axdr_get_length(&probe) >= 2) {
            *in = probe; /* accept the standard structure wrapper */
        }
        uint8_t user_id = 0;
        if (read_typed_u8(in, ND_AXDR_UNSIGNED, &user_id) != 0 || user_id == 0
            || user_id > state->user_count) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->users[user_id - 1][0] = 0;
        state->user_len[user_id - 1] = 0;
        return ND_OK;
    }
    return ND_ERR_OPERATION_NOT_POSSIBLE;
}

static int security_action(nd_security_state_t *state, uint8_t method_id,
                           nd_axdr_reader_t *in, nd_axdr_writer_t *out,
                           const nd_crypto_port_t *crypto)
{
    if (method_id == 1) { /* security_activate */
        uint8_t value = 0;
        if (read_typed_u8(in, ND_AXDR_ENUM, &value) != 0) {
            return ND_ERR_TYPE_MISMATCH;
        }
        state->activated = 1;
        return ND_OK;
    }
    if (method_id == 2 || method_id == 3) { /* key transfer / agreement */
        uint8_t key[16];
        uint8_t length = 0;
        if (nd_axdr_get_tag(in) == ND_AXDR_STRUCTURE
            && nd_axdr_get_length(in) >= 2) {
            uint8_t key_id = 0;
            if (read_typed_u8(in, ND_AXDR_ENUM, &key_id) != 0) {
                return ND_ERR_TYPE_MISMATCH;
            }
            (void)key_id;
        }
        if (read_octets(in, key, sizeof(key), &length) != 0 || length != 16) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint8_t slot = method_id == 2 ? 0 : 1;
        memcpy(state->keys[slot], key, 16);
        state->key_len[slot] = 16;
        return ND_OK;
    }
    if (method_id == 4) { /* generate_key_pair */
        uint8_t public_key[64];
        size_t public_len = sizeof(public_key);
        if (crypto == NULL || crypto->key_pair_generate == NULL
            || crypto->key_pair_generate(crypto->ctx, public_key,
                                         &public_len) != 0) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        return ND_OK;
    }
    if (method_id == 5) { /* generate_certificate_request */
        uint8_t public_key[64];
        size_t public_len = sizeof(public_key);
        uint8_t request[128];
        size_t request_len = sizeof(request);
        if (crypto == NULL || crypto->key_pair_generate == NULL
            || crypto->certificate_request == NULL
            || crypto->key_pair_generate(crypto->ctx, public_key,
                                         &public_len) != 0
            || crypto->certificate_request(crypto->ctx, public_key,
                                           public_len, request,
                                           &request_len) != 0) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        if (out != NULL) {
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, (uint16_t)request_len);
            nd_axdr_put_octets(out, request, request_len);
        }
        return ND_OK;
    }
    if (method_id == 6) { /* import_certificate */
        return read_octets(in, state->cert, sizeof(state->cert),
                           &state->cert_len) == 0
                   ? ND_OK
                   : ND_ERR_TYPE_MISMATCH;
    }
    if (method_id == 7) { /* export_certificate */
        if (out == NULL) {
            return ND_ERR_OPERATION_NOT_POSSIBLE;
        }
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, state->cert_len);
        nd_axdr_put_octets(out, state->cert, state->cert_len);
        return nd_axdr_writer_ok(out) ? ND_OK
                                      : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (method_id == 8) { /* remove_certificate */
        state->cert_len = 0;
        return ND_OK;
    }
    return ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_association_action(void *ctx, uint8_t client_sap,
                             const nd_obis_t *ln, uint8_t method_id,
                             nd_axdr_reader_t *in, nd_axdr_writer_t *out)
{
    nd_idis_association_t *state = ctx;
    (void)client_sap;
    if (state == NULL || ln == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (nd_obis_eq(ln, &ND_OBIS_ASSOCIATION)) {
        return assoc_action(&state->association, method_id, in, out,
                            state->crypto);
    }
    if (nd_obis_eq(ln, &ND_OBIS_SAP_ASSIGNMENT)) {
        return method_id == 1 ? ND_OK : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (nd_obis_eq(ln, &ND_OBIS_SECURITY_SETUP)) {
        return security_action(&state->security, method_id, in, out,
                               state->crypto);
    }
    if (nd_obis_eq(ln, &ND_OBIS_SECURITY_SETUP_CONSUMER)) {
        return security_action(&state->consumer, method_id, in, out,
                               state->crypto);
    }
    return ND_ERR_OBJECT_UNDEFINED;
}
