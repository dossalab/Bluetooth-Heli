#include <nfc/t4t_lib/nfc_t4t_lib.h>
#include <nfc/ndef/uri/nfc_uri_msg.h>
#include <app_error.h>
#include <stddef.h> /* for NULL */

#define DEFAULT_NFC_URL		"https://cringe.page"

static void nfc_callback(void *user, nfc_t4t_event_t event,
		const uint8_t *data, size_t len, uint32_t flags)
{
	/* We don't need this for now... */

	switch (event) {
	default:
		break;
	}
}

static void nfc_encode_payload(const char *url)
{
	ret_code_t err;
	uint32_t payload_len;
	static uint8_t nfc_payload_buffer[256];

	err = nfc_uri_msg_encode(NFC_URI_HTTP_WWW, (const uint8_t *)url,
		strlen(url), nfc_payload_buffer, &payload_len);
	APP_ERROR_CHECK(err);

	err = nfc_t4t_ndef_staticpayload_set(nfc_payload_buffer, payload_len);
	APP_ERROR_CHECK(err);

}

void nfc_init_with_url(const char *url)
{
	ret_code_t err;

	err = nfc_t4t_setup(nfc_callback, NULL);
	APP_ERROR_CHECK(err);

	nfc_encode_payload(url);

	err = nfc_t4t_emulation_start();
	APP_ERROR_CHECK(err);
}

void nfc_init(void)
{
	nfc_init_with_url(DEFAULT_NFC_URL);
}
