#pragma once

#include <openssl/evp.h>
#include <openssl/aes.h>
#include <string.h>

namespace avs_toolkit
{
    class AES256CBC
    {
    public:
        using Ptr = std::shared_ptr<AES256CBC>;

        explicit AES256CBC(const char* key, const char* iv) :
            m_key(key),
            m_iv(iv)
        {
        }

        virtual ~AES256CBC() {}

        int getAES256CBCSize(int intput_size)
        {
            return ((intput_size + AES_BLOCK_SIZE) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;;
        }

        // ���ܺ���
        int encrypt(const char* plaintext, int plaintext_len, char* ciphertext)
        {
            // ��ʼ������������
            EVP_CIPHER_CTX* ctx;
            if (!(ctx = EVP_CIPHER_CTX_new()))
            {
                return -1;
            }

            // ��ʼ�����ܲ���
            if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (const unsigned char*)m_key, (const unsigned char*)m_iv))
            {
                return -1;
            }

            // ��������
            int len = 0;
            int ciphertext_len = 0;
            if (1 != EVP_EncryptUpdate(ctx, (unsigned char*)ciphertext, &len, (const unsigned char*)plaintext, plaintext_len))
            {
                return -1;
            }
            ciphertext_len = len;

            // ��ɼ���
            if (1 != EVP_EncryptFinal_ex(ctx, (unsigned char*)ciphertext + len, &len))
            {
                return -1;
            }
            ciphertext_len += len;

            // �ͷ���Դ
            EVP_CIPHER_CTX_free(ctx);

            // ���ؼ��ܺ�����ݳ���
            return ciphertext_len;
        }

        // ���ܺ���
        int decrypt(const char* ciphertext, int ciphertext_len, char* plaintext)
        {
            // ��ʼ������������
            EVP_CIPHER_CTX* ctx;
            if (!(ctx = EVP_CIPHER_CTX_new()))
            {
                return -1;
            }

            if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (const unsigned char*)m_key, (const unsigned char*)m_iv))
            {
                return -1;
            }

            // ��������
            int len = 0;
            int plaintext_len = 0;
            if (1 != EVP_DecryptUpdate(ctx, (unsigned char*)plaintext, &len, (const unsigned char*)ciphertext, ciphertext_len))
            {
                return -1;
            }
            plaintext_len = len;

            // ��ɽ���
            if (1 != EVP_DecryptFinal_ex(ctx, (unsigned char*)plaintext + len, &len))
            {
                return -1;
            }
            plaintext_len += len;

            // �ͷ���Դ
            EVP_CIPHER_CTX_free(ctx);

            // ���ؽ��ܺ�����ݳ���
            return plaintext_len;
        }

    private:
        // ������Կ�� IV
        const char* m_key;
        const char* m_iv;
    };

}
