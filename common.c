//
//  Created by Boris Stasenko on 07.12.2019
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#pragma ide diagnostic ignored "bugprone-narrowing-conversions"
#pragma clang diagnostic ignored "-Wunknown-pragmas"


#include "common.h"


static char *time_format = "[%d.%m.%Y %H:%M:%S] >>> ";
static int time_format_len = 27;


//  логирование
void p_log(enum LOG_TYPE type, char *format, ...) {
    char out_buf[256];

    if (!format) {
        sprintf(out_buf, "ERROR: %s\n", strerror(errno));
    } else {
        va_list ptr;
        va_start(ptr, format);
        vsprintf(out_buf, format, ptr);
        va_end(ptr);
    }

    if (logger) {
        time_t raw_time;
        struct tm *time_info;
        char buffer[time_format_len + 1];

        time(&raw_time);
        time_info = localtime(&raw_time);
        strftime(buffer, time_format_len, time_format, time_info);
        buffer[time_format_len + 1] = '\0';

        write(fileno(logger), buffer, strlen(buffer));
        write(fileno(logger), " ", 1);
        write(fileno(logger), out_buf, strlen(out_buf));
    }

    if (type == DOUBLE_LOG) {
        write(fileno(format ? stdout : stderr), out_buf, strlen(out_buf));
    }
}


//  используется для преобразования 3-байтовых последовательностей в символы
const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


/*
 * В кодировке base64 три двоичных байта представлены как четыре символа
 * ---------------------------------------------------------------------
 * Чтобы получить количество символов base64, нужно:
 * 1) сначала взять длину ввода и округлить до ближайшего кратного 3
 * 2) затем разделить на 3, чтобы получить количество 3-байтовых блоков
 * 3) и в конце умножить на 4, чтобы получить количество символов base64
 */
size_t b64_encoded_size(size_t in_len) {
    size_t ret;
    ret = in_len;

    if (in_len % 3 != 0) ret += 3 - (in_len % 3);
    ret /= 3;
    ret *= 4;

    return ret;
}


/*
 * Функция кодирования работает в блоках по 3-двоичных байта данных,
 * чтобы превратить их в символы base64 ASCII
 */
char *b64_encode(const unsigned char *in, size_t len) {
    char *out;

    size_t e_len;
    size_t i;
    size_t j;
    size_t v;

    if (in == NULL || len == 0) return NULL;

    e_len = b64_encoded_size(len);
    out = malloc(e_len + 2);

    out[e_len] = '\n';
    out[e_len + 1] = '\0';

    for (i = 0, j = 0; i < len; i += 3, j += 4) {

        //  помещаем 3 байта в int и, если есть менее 3 байтов, просто
        //  сдвигаем значение, чтобы мы смогли правильно извлечь его позже
        v = in[i];
        v = i + 1 < len ? v << 8 | in[i + 1] : v << 8;
        v = i + 2 < len ? v << 8 | in[i + 2] : v << 8;


        //  вытаскиваем символ base64 из сопоставления, где используется 0x3F,
        //  потому что всего 6 бит, где все установлено и поскольку каждый выходной байт
        //  разделен на серию из 6 битов, нам нужно отобразить только те биты, которые помещены в данный выходной байт
        out[j] = b64chars[(v >> 18) & 0x3F];
        out[j + 1] = b64chars[(v >> 12) & 0x3F];

        //  добавляем байт или '=' для заполнения, если в последнем блоке меньше 3 байтов
        out[j + 2] = i + 1 < len ? b64chars[(v >> 6) & 0x3F] : '=';
        out[j + 3] = i + 2 < len ? b64chars[v & 0x3F] : '=';
    }

    return out;
}

#pragma clang diagnostic pop