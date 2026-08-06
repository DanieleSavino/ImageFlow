#include "ImageFlow/operations/operations.h"
#include "ImageFlow/error.h"
#include "ImageFlow/io/image.h"

IF_OpImpl_t *IF_OpImpls[_IF_OP_LEN][_IF_DEV_LEN];

NODISCARD IF_error_t IF_op_execute(IF_Operation_t *op, IF_image_t **imgs) {
    IF_OpImpl_t *op_impl = IF_op_get_impl(op->supp_op, op->pref_dev);

    if (op_impl == NULL)
        return IF_INVALID_ARGS;

    return op_impl->func(op->op_args, imgs);
}

/* String names, built from the same list */
#define IF_OP_DEF(op, name, type, args, def) #name,

const char *IF_OpNames[_IF_OP_LEN] = {
    #include "ImageFlow/operations/operations.def"
};

#undef IF_OP_DEF

#define IF_OP_DEF(op, name, type, args, def) IF_TRAVERSAL_##type,

const IF_OpType_t IF_OpTypes[_IF_OP_LEN] = {
    #include "ImageFlow/operations/operations.def"
};

#undef IF_OP_DEF
