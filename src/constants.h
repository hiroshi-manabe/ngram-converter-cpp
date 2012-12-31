#ifndef NGRAM_CONVERTER_CONSTANTS_H_INCLUDED_
#define NGRAM_CONVERTER_CONSTANTS_H_INCLUDED_

#define BLOCK_SIZE 128
#define PAIR_SEPARATOR "/"
#define PAIR_SEPARATOR_LEN (sizeof(PAIR_SEPARATOR) - 1)
#define MAX_KEY_LEN 255
#define MAX_N 10
#define BOS_STR "<s>"
#define EOS_STR "</s>"
#define UNK_STR "UNK"
#define MAX_INFLECTION 40

#define MAX_INT_SCORE 255
#define BACKOFF_DISCOUNT 128
#define FLAGS_HAS_TOKEN_IDS 1
#define FLAGS_HAS_BACKOFF 2
#define MIN_SCORE -7.0
#define INVALID_SCORE -65536.0
#define INVALID_CODE 0xFFFFFFFF

#define EXT_KEY ".key"
#define EXT_PAIR ".pair"
#define EXT_INDEX ".index"
#define EXT_DATA ".data"
#define EXT_FILTER ".filter"
#define EXT_TEKEI ".tekei"
#define EXT_VERB_KEY ".verbkey"
#define EXT_VERB_DATA ".verbdata"

#define FILTER_COUNT 7
#define FILTER_BITS_PER_ELEM 10

#endif  // NGRAM_CONVERTER_CONSTANTS_H_INCLUDED_
