#include <cJSON.h>
#include <ctype.h>
#include <curl/curl.h>
#include <math.h>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/ossl_typ.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CRYPTO_DEBUG 0

static unsigned char aesIv[] = "0102030405060708";
static unsigned char presetKey[] = "0CoJUm6Qyw8W8jud";
static unsigned char linuxapiKey[] = "rFgB&h#%2?^eDg:Q";
static unsigned char base62[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static unsigned char publicKey[] =
	"-----BEGIN PUBLIC "
	"KEY-----\nMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDgtQn2JZ34ZC28NWYpAUd98iZ37BUrX/"
	"aKzmFbt7clFSs6sXqHauqKWqdtLkF2KexO40H1YTX8z2lSgBBOAxLsvaklV8k4cBFK9snQXE9/"
	"DDaFt6Rr7iVZMldczhC0JNgTz+SHXT6CBHuX3e9SdB1Ua44oncaTWz7OBGLbCiK45wIDAQAB\n-----END PUBLIC KEY-----";
static const char eapiKey[] = "e82ckenh8dichen8";
/**
 * @param input the input string
 * @param inputLenght length of input string
 * @param outputLength length of output base64 str
 * @return the pointer to the output base64 string
 * note the returned string contain a 0 character in the end
 */
unsigned char *base64(unsigned char *input, int inputLength, size_t *outputLength) {
	BIO *b64 = NULL;
	BIO *bmem = NULL;
	BUF_MEM *bptr = NULL;
	char *output = NULL;
	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bmem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, bmem);
	BIO_write(b64, input, inputLength);
	BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);

	output = (char *)calloc(bptr->length + 1, sizeof(char));
	memcpy(output, bptr->data, bptr->length);
	output[bptr->length] = 0;
	BIO_free_all(b64);
	*outputLength = bptr->length;
	return output;
}
/** aes_128_encrypt
 * This function will allocate memory and store the encrypted cipher in it
 * @param key key used to encrypt in AES-128
 * @param iv initialization vector in AES-128
 * @param inputData pointer to input data
 * @param inputSize size of input data to be encrypted
 * @param outputData pointer to outputData array
 * @return size of encrypted cipher buffer
 * Note that outputData needs to be manually freed
 **/
static int aes_128_encrypt(unsigned char *key, int isCBC, unsigned char *iv, unsigned char *inputData, int inputSize,
						   unsigned char **outputData) {
	EVP_CIPHER_CTX *keyCtx;
	int len = 0;
	int final_len = 0;

	// Calculate cipher output size in advance
	int sizeCipherBuffer = (int)ceilf((float)inputSize / 16) * 16;
	// if the size of an input block is a multiple of 16, it will add another padding block following PKCS#7 rule
	// https://crypto.stackexchange.com/questions/12621/why-does-openssl-append-extra-bytes-when-encrypting-with-aes-128-ecb
	if (inputSize == sizeCipherBuffer) {
		sizeCipherBuffer += 16;
	}
	*outputData = (unsigned char *)malloc(sizeCipherBuffer);
	// Create and initialize the context
	keyCtx = EVP_CIPHER_CTX_new();
	if (isCBC) {
		EVP_EncryptInit_ex(keyCtx, EVP_aes_128_cbc(), NULL, key, iv);
	} else {
		EVP_EncryptInit_ex(keyCtx, EVP_aes_128_ecb(), NULL, key, 0);
	}

	// Set padding mode (PKCS#5 padding is used by default)
	EVP_CIPHER_CTX_set_padding(keyCtx, 1); // 1 for standard padding
	// Perform encryption
	EVP_EncryptUpdate(keyCtx, *outputData, &len, inputData, inputSize);

	// Finalize the encryption (if necessary)
	EVP_EncryptFinal_ex(keyCtx, *outputData + len, &final_len);

	// Clean up
	EVP_CIPHER_CTX_free(keyCtx);
	return len + final_len;
}
/** NCM_eapi
 * @param object cJSON object as an input
 * @param pszUrl requested URL
 * @param dataParam output data param string
 * @return -1 when curl_easy_escape failed, otherwise 0 when successful
 * need to free cJSON parameter `object` manually after invokation of this function
 * variables stored in heap : encryptedDataBuffer base64str rsaCipherSourceBuffer rsaCipherDestBuffer
 * need to manually
 **/
int NCM_eapi(cJSON *object, const char *pszUrl, char **dataParam) {

	// stringify object
	char *pszObjStr = cJSON_Print(object);
#if CRYPTO_DEBUG
	printf("raw data after stringify:\n%s\n", pszObjStr);
#endif
	size_t nObjStrLength = strlen(pszObjStr);
	size_t nUrlLength = strlen(pszUrl);
	size_t nBufferSize = nObjStrLength + nUrlLength + strlen("nobodyusemd5forencrypt") + 1;
	char *pszText = (char *)malloc(nBufferSize);
	sprintf(pszText, "nobody%suse%smd5forencrypt", pszUrl, pszObjStr);
#if CRYPTO_DEBUG
	printf("text to be md5 hashed:\n%s\n", pszText);
#endif

	// allocate memory for hash
	unsigned char *hash = (unsigned char *)malloc(128 / 8);
	char *pszHashHex = (unsigned char *)malloc(128 / 8 * 2 + 1);

	// Init MD5 Context and perform hash operation
	MD5_CTX md5Ctx;
	MD5_Init(&md5Ctx);
	MD5_Update(&md5Ctx, pszText, nBufferSize - 1);
	MD5_Final(hash, &md5Ctx);
	free(pszText);

	// convert to hex
	for (int i = 0; i < 16; i++) {
		unsigned char tmp = hash[i];

		if ((tmp & 0xf) < 10) {
			pszHashHex[i * 2 + 1] = '0' + (tmp & 0xf);
		} else {
			pszHashHex[i * 2 + 1] = 'a' + (tmp & 0xf) - 10;
		}
		tmp >>= 4;

		if ((tmp & 0xf) < 10) {
			pszHashHex[i * 2] = '0' + (tmp & 0xf);

		} else {
			pszHashHex[i * 2] = 'a' + (tmp & 0xf) - 10;
		}
	}
	pszHashHex[32] = 0;
	free(hash);
#if CRYPTO_DEBUG
	printf("MD5 in hex:%s\n", pszHashHex);
#endif
	// compose the final raw data
	size_t nSizeData = strlen("-36cd479b6b5-") * 2 + nObjStrLength + nUrlLength + 16 * 2 + 1;
	char *pszData = (char *)malloc(nSizeData);
	sprintf(pszData, "%s-36cd479b6b5-%s-36cd479b6b5-%s", pszUrl, pszObjStr, pszHashHex);

	// free(pszObjStr);
#if CRYPTO_DEBUG
	printf("after md5:\n%s\n", pszData);
#endif
	free(pszHashHex);
	unsigned char *cipher;

	int nCipherSize = aes_128_encrypt(eapiKey, 0, 0, pszData, nSizeData - 1, &cipher);
	free(pszData);
	*dataParam = (char *)malloc(nCipherSize * 2 + 7 + 1);

	char *destData = *dataParam;
	// to upper case hex
	// since the params is already in HEX, we won't do URL encoding
	strcpy_s(destData, 8, "params=");
	for (int i = 0; i < nCipherSize; i++) {
		unsigned char tmp = cipher[i];
		if ((tmp & 0xf) < 10) {
			destData[i * 2 + 1 + 7] = '0' + (tmp & 0xf);
		} else {
			destData[i * 2 + 1 + 7] = 'A' + (tmp & 0xf) - 10;
		}
		tmp >>= 4;

		if ((tmp & 0xf) < 10) {
			destData[i * 2 + 7] = '0' + (tmp & 0xf);

		} else {
			destData[i * 2 + 7] = 'A' + (tmp & 0xf) - 10;
		}
	}
	destData[nCipherSize * 2 + 7] = 0;
	free(cipher);
#if CRYPTO_DEBUG
	printf("final post fields:\n%s\n", *dataParam);
#endif
	return 0;
}
int NCM_decrypt() {}
/** weapi
 * @param object cJSON object as an input
 * @param dataParam output data param string
 * @return -1 when curl_easy_escape failed, otherwise 0 when successful
 * need to free cJSON parameter `object` manually after invokation of this function
 * variables stored in heap : encryptedDataBuffer base64str rsaCipherSourceBuffer rsaCipherDestBuffer
 * need to manually
 **/
int NCM_weapi(cJSON *object, char **dataParam) {

	// stringify
	char *objectString = cJSON_Print(object);
	size_t objectStringLength = strlen(objectString);
#if CRYPTO_DEBUG
	printf("raw data after stringify:\n%s\n", objectString);
#endif
	// create secretKey and presetKeyCtx(Context)
	unsigned char secretKey[16];
	unsigned char secretKeyReversed[16];
	RAND_bytes(secretKey, 16);
	for (int i = 0; i < 16; i++) {
		secretKey[i] = (unsigned char)base62[secretKey[i] % 62];
	}
	BUF_reverse(secretKeyReversed, secretKey, 16);

	unsigned char *encryptedDataBuffer;
	int aesCipherLength = aes_128_encrypt(presetKey, 1, aesIv, objectString, objectStringLength, &encryptedDataBuffer);
#if CRYPTO_DEBUG
	printf("aesCipherLength=%d\n", aesCipherLength);
#endif
	// perform 1st stage AES encrypt
	// Remember not to include the terminator zero
	size_t base64strLength;
	unsigned char *base64str = base64(encryptedDataBuffer, aesCipherLength, &base64strLength);
#if CRYPTO_DEBUG
	printf("1st AES encrypted in base64 form:%s\n", base64str);
#endif
	free(encryptedDataBuffer);

	// Perform 2st stage AES encrypt
	// Remember not to include the terminator zero
	aesCipherLength = aes_128_encrypt(secretKey, 1, aesIv, base64str, base64strLength, &encryptedDataBuffer);
	free(base64str);
	base64str = base64(encryptedDataBuffer, aesCipherLength, &base64strLength);
	free(encryptedDataBuffer);
#if CRYPTO_DEBUG
	printf("2nd AES encrypted in base64 form:%s\n", base64str);
#endif

	// read RSA key buffer into BIO
	RSA *rsaPubkey = NULL;
	BIO *rsaKeyBIO = BIO_new_mem_buf(publicKey, -1);
	rsaPubkey = PEM_read_bio_RSA_PUBKEY(rsaKeyBIO, &rsaPubkey, NULL, NULL);

	// printf("former secretKey:%s\n", secretKey);

	// printf("latter secretKey:%s\n", secretKeyReversed);
	unsigned char *rsaCipherSourceBuffer = (unsigned char *)malloc(128);
	unsigned char *rsaCipherDestBuffer = (unsigned char *)malloc(1024 / 8);
	unsigned char *hexDestBuffer = (unsigned char *)malloc(1024 / 8 * 2 + 1);
	memset(rsaCipherSourceBuffer, 0, 128);
	memcpy(rsaCipherSourceBuffer + 128 - 16, secretKeyReversed, 16);
	RSA_public_encrypt(128, rsaCipherSourceBuffer, rsaCipherDestBuffer, rsaPubkey, RSA_NO_PADDING);
	// convert to hex
	for (int i = 0; i < 128; i++) {
		unsigned char tmp = rsaCipherDestBuffer[i];

		if ((tmp & 0xf) < 10) {
			hexDestBuffer[i * 2 + 1] = '0' + (tmp & 0xf);
		} else {
			hexDestBuffer[i * 2 + 1] = 'a' + (tmp & 0xf) - 10;
		}
		tmp >>= 4;

		if ((tmp & 0xf) < 10) {
			hexDestBuffer[i * 2] = '0' + (tmp & 0xf);

		} else {
			hexDestBuffer[i * 2] = 'a' + (tmp & 0xf) - 10;
		}
	}
	free(rsaCipherSourceBuffer);
	free(rsaCipherDestBuffer);
	hexDestBuffer[1024 / 8 * 2] = 0;
#if CRYPTO_DEBUG
	printf("RSA encrypted in hex form:%s\n", hexDestBuffer);
#endif
	// Do url encoding with percentage before concatenating

	char *base64strUrlEncoded;
	CURL *curl = curl_easy_init();
	if (curl) {
		base64strUrlEncoded = curl_easy_escape(curl, base64str, base64strLength);
		free(base64str);
		curl_easy_cleanup(curl);
	} else {
		printf("libcurl failed to invoke curl_easy_escape.\n");
		free(base64str);
		free(hexDestBuffer);
		return -1;
	}
	// concatenate two params
	*dataParam = (char *)malloc(1024 / 8 * 2 + strlen(base64strUrlEncoded) + 18 + 1); // 2 params + characters + terminator
	sprintf(*dataParam, "params=%s&encSecKey=%s", base64strUrlEncoded, hexDestBuffer);
	free(hexDestBuffer);

	cJSON_free(objectString);
	// successfully executed
	return 0;
}