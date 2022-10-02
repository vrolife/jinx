#include <jinx/openssl/bio.hpp>

namespace jinx {
namespace openssl {

static char event_engine_bio_type_tag = 0;

static int bio_type = 0;
static BIO_METHOD* bio_method = nullptr;

ResultGeneric StreamBIO::initialize() {
    if (JINX_UNLIKELY(bio_method == nullptr)) {
        bio_type = BIO_get_new_index();
        bio_method = BIO_meth_new(bio_type, "stream-bio");
        BIO_meth_set_read_ex(bio_method, read);
        BIO_meth_set_write_ex(bio_method, write);
        BIO_meth_set_ctrl(bio_method, ctrl);
    }

    _bio.reset(BIO_new(bio_method));
    if (_bio == nullptr) {
        return Failed_;
    }

    BIO_set_app_data(_bio, this);
    return Successful_;
}

const void* EventEngineBIO::get_type_tag() {
    return &event_engine_bio_type_tag;
}

namespace detail {

ResultGeneric BIOEventCallback::remove() {
    return _stream->remove_io(this);
}

} // namespace detail

} // namespace openssl
} // namespace jinx
