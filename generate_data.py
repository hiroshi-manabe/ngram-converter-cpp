#!/usr/bin/python
# -*- coding: utf-8 -*-

# built-ins
import getopt
import os
import struct
import sys

BLOCK_SIZE = 128
MAX_INT_SCORE = 255
BACKOFF_DISCOUNT = 128
FLAGS_HAS_TOKEN_IDS = 1
FLAGS_HAS_BACKOFF = 2

EXT_INDEX = '.index'
EXT_DATA = '.data'

def Usage():
    print sys.argv[0] + ' - generate N-gram data from a sorted N-gram file.'
    print 'Usage: '  + sys.argv[0] + \
        '(-i <input filename>|-v <verification filename) ' + \
        '-o <output filename prefix> ' + \
        '-s <min score> '
    sys.exit()


def gen_test():
    for i in range(128):
        yield 1

class BitArray(object):
    def __init__(self):
        self._bytearray = bytearray()
        self._length = 0

    def AddBits(self, bit_seq):
        for b in bit_seq:
            bit = 1 if b else 0

            if self._length % 8 == 0:
                self._bytearray.append(0)

            self._bytearray[-1] |= bit << (self._length % 8)
            self._length += 1

    def IterBits(self):
        for i in range(self._length):
            yield 1 if self._bytearray[int(i / 8)] & (1 << (i % 8)) else 0

    def GetByteArray(self):
        return self._bytearray

    def SetByteArray(self, b_array, length):
        if int(length / 8) != len(b_array):
            raise ValueError

        self._bytearray = bytearray(b_array)
        self._length = length


def EncodeNumber(x):
    b = bytearray()
    if x < (1 << 7):
        b.append((1 << 7) | x)
    elif x < (1 << 14):
        b.append((1 << 6) | (x >> 8))
        b.append(x & 0xff)
    elif x < (1 << 21):
        b.append((1 << 5) | (x >> 16))
        b.append((x >> 8) & 0xff)
        b.append(x & 0xff)
    elif x < (1 << 28):
        b.append((1 << 4) | (x >> 24))
        b.append((x >> 16) & 0xff)
        b.append((x >> 8) & 0xff)
        b.append(x & 0xff)

    return b


def DecodeNumber(b):
    length = 0;
    x = 0
    if b[0] & (1 << 7):
        x = b[0] & ((1 << 7) - 1)
        length = 1
    elif b[0] & (1 << 6):
        x = ((b[0] & ((1 << 6) - 1)) << 8) | b[1]
        length = 2
    elif b[0] & (1 << 5):
        x = ((b[0] & ((1 << 5) - 1)) << 16) | (b[1] << 8) | b[2]
        length = 3
    elif b[0] & (1 << 4):
        x = ((b[0] & ((1 << 4) - 1)) << 24) | (b[1] << 16) | (b[2] << 8) | b[3]
        length = 4

    return (x, length)


class NgramBlock(object):
    def __init__(self, max_size):
        self._store_token_data = False
        self._store_backoff_data = False

        self._score_data = bytearray(max_size)
        self._backoff_bits = BitArray()
        self._backoff_data = bytearray()

        self._token_data = BitArray()
        self._context_data = bytearray()

        self._size = 0
        self._max_size = max_size

        self._first_token_id = None
        self._first_context_id = None
        self._prev_token_id = None
        self._prev_context_id = None

    def AddNgram(self, token_id, context_id, score_int, backoff_int):
        if self._size >= self._max_size:
            raise ValueError

        self._score_data[self._size] = score_int

        has_backoff = backoff_int != -1
        self._backoff_bits.AddBits((has_backoff,))
        if has_backoff:
            self._backoff_data.append(backoff_int)
            self._store_backoff_data = True

        self._size += 1

        if self._first_token_id is None:
            self._first_token_id = self._prev_token_id = token_id
            self._first_context_id = self._prev_context_id = context_id
            return self._size >= self._max_size

        if token_id != self._prev_token_id:
            self._store_token_data = True

        self._token_data.AddBits([0] * (token_id - self._prev_token_id) + [1])
        self._context_data += EncodeNumber(
            context_id - (self._prev_context_id if
                          self._prev_token_id == token_id else 0))

        self._prev_token_id = token_id
        self._prev_context_id = context_id

        return self._size >= self._max_size

    def GetFirstIds(self):
        return (self._first_token_id, self._first_context_id)

    def FlushData(self):
        data = bytearray()

        if self._size == 0:
            return data

        for i in range(self._max_size - self._size):
            self.AddNgram(self._prev_token_id, self._prev_context_id, 0, 0)

        data.append(self._store_backoff_data * FLAGS_HAS_BACKOFF |
                    self._store_token_data * FLAGS_HAS_TOKEN_IDS)

        data += self._score_data

        if self._store_backoff_data:
            data += self._backoff_bits.GetByteArray()
            data += self._backoff_data

        if self._store_token_data:
            data += self._token_data.GetByteArray()

        data += self._context_data

        self.__init__(self._max_size)
        return data


def EncodeScore(x, min_score):
    return int(x * MAX_INT_SCORE / min_score)


def DecodeScore(x, min_score):
    return x * min_score / MAX_INT_SCORE


def DecodeFiles(output_filename_prefix, verification_filename, block_size,
                min_score):
    def decode_block(buf, block_size, token_ids, context_ids, scores, backoffs):
        offset = 0
        flags = buf[offset]
        offset += 1

        for i in range(block_size):
            scores[i] = DecodeScore(buf[offset], min_score)
            offset += 1

        if flags & FLAGS_HAS_BACKOFF:
            backoff_bits = BitArray()
            backoff_bits.SetByteArray(buf[offset:offset + int(block_size / 8)],
                                      block_size)
            offset += int(block_size / 8)

            i = 0
            for b in backoff_bits.IterBits():
                if b:
                    backoffs[i] = DecodeScore(buf[offset] -
                                              BACKOFF_DISCOUNT,
                                              min_score)
                    offset += 1
                else:
                    backoffs[i] = None

                i += 1

        if flags & FLAGS_HAS_TOKEN_IDS:
            token_block_len = 0
            token_bits = BitArray()
            token_bits.SetByteArray(buf[offset:], len(buf[offset:]) * 8)

            i = 1
            bit_len = 0
            count = 0

            for b in token_bits.IterBits():
                bit_len += 1
                if b:
                    token_ids[i] = token_ids[i-1] + count
                    token_block_len += count + 1
                    count = 0
                    i += 1
                    if i >= block_size:
                        break

                else:
                    count += 1

            offset += int((token_block_len + 7) / 8)

        else: # flags & FLAGS_HAS_TOKEN_IDS
            token_ids[1:block_size] = [token_ids[0]] * (block_size - 1)

        for i in range(1, block_size):
            (context_id_offset, length) = DecodeNumber(buf[offset:])
            context_ids[i] = context_id_offset

            if token_ids[i] == token_ids[i-1]:
                context_ids[i] += context_ids[i-1]

            offset += length

        if offset != len(buf):
            print 'token_ids: %s context_ids: %s offset: %d len(buf): %d' % (
                (token_ids,), (context_ids,), offset, len(buf))
            raise ValueError

        return

    f_in_index = open(output_filename_prefix + EXT_INDEX, 'rb')
    f_in_data = open(output_filename_prefix + EXT_DATA, 'rb')
    f_out = open(verification_filename, 'w')

    cur_n = 0
    cur_ngram_count = 0

    while True:
        cur_n += 1
        buf = f_in_index.read(4)
        if not buf:
            break

        cur_ngram_count = struct.unpack('=L', buf)[0]
        f_out.write('#%d\t%d\n' % (cur_n, cur_ngram_count))
        print 'Processing %d-gram...' % cur_n

        prev_data_offset = f_in_data.tell()
        data_offset = None

        token_ids = [0] * block_size
        context_ids = [0] * block_size
        scores_int = [0] * block_size
        scores = [0.0] * block_size
        backoffs_int = [0] * block_size
        backoffs = [None] * block_size

        for i in range(cur_ngram_count):
            if cur_n == 1:
                buf = bytearray(f_in_index.read(2))
                score = DecodeScore(buf[0], min_score)
                backoff = DecodeScore(buf[1] - BACKOFF_DISCOUNT, min_score)
                f_out.write('%d\t%d\t%d\t%f\t%f\n' % (i, i, 0, score, backoff))

            else: # cur_n != 1
                block_index = i % block_size
                if block_index == 0:
                    buf = f_in_index.read(12)
                    (token_ids[0], context_ids[0], data_offset) = struct.unpack('=LLL', buf)
                    if prev_data_offset != f_in_data.tell():
                        raise ValueError

                    data_buf = bytearray(f_in_data.read(
                            data_offset - prev_data_offset))
                    decode_block(data_buf, block_size, token_ids, context_ids,
                                 scores, backoffs)
                    prev_data_offset = data_offset

                f_out.write('%d\t%d\t%d\t%f\t%f\n' % (
                        i,
                        token_ids[block_index],
                        context_ids[block_index],
                        scores[block_index],
                        -99.0 if backoffs[block_index] is None
                        else backoffs[block_index]))

    print 'Done.'

def main():
    def output_data(f_out_index, f_out_data, ngram_block):
        (first_token_id, first_context_id) = ngram_block.GetFirstIds()

        data = ngram_block.FlushData()
        f_out_data.write(data)

        f_out_index.write(struct.pack('=LLL', first_token_id,
                                      first_context_id, f_out_data.tell()))
        
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hi:o:s:v:')
    except getopt.GetoptError:
        Usage()
        sys.exit(2)

    input_filename = ''
    output_filename_prefix = ''
    verification_filename = ''
    flag_verification_only = False
    min_score = 0.0

    for k, v in opts:
        if k == '-h':
            usage()
            sys.exit()
        elif k == '-d':
            flag_verification_only = True
        elif k == '-i':
            input_filename = v
        elif k == '-o':
            output_filename_prefix = v
        elif k == '-s':
            min_score = float(v)
        elif k == '-v':
            verification_filename = v

    if not output_filename_prefix or not min_score or (
        not input_filename and not verification_filename):
        Usage()

    if not input_filename and verification_filename:
        DecodeFiles(output_filename_prefix, verification_filename, BLOCK_SIZE,
                    min_score)
        sys.exit(0)

    if not input_filename:
        Usage()

    f_in = open(input_filename, 'r')
    f_out_index = open(output_filename_prefix + EXT_INDEX, 'wb')
    f_out_data = open(output_filename_prefix + EXT_DATA, 'wb')

    cur_n = 0
    cur_ngram_count = 0
    ngram_block = NgramBlock(BLOCK_SIZE)
    was_full = False

    for line in f_in:
        line = line.rstrip('\n')
        if not line:
            continue

        fields = line.split('\t')

        if fields[0][0] == '#':
            if cur_n > 1:
                output_data(f_out_index, f_out_data, ngram_block)

            new_n = int(fields[0][1:])
            if new_n != cur_n + 1:
                raise ValueError

            cur_n = new_n
            cur_ngram_count = int(fields[1])
            print 'Processing %d-gram...' % cur_n

            f_out_index.write(struct.pack('=L', cur_ngram_count))
            
            continue

        (id, token_id, context_id) = (int(x) for x in fields[:3])
        (score, backoff) = (float(x) for x in fields[3:5])

        if cur_n == 1 and backoff == -99.0:
            backoff = 0.0

        (score_int, backoff_int) = (EncodeScore(x, min_score) for x
                                    in (score, backoff))

        backoff_int += BACKOFF_DISCOUNT

        if score_int < 0:
            score_int = 0
        if score_int > 255:
            score_int = 255
        if backoff_int < 0:
            backoff_int = 0
        if backoff_int > 255:
            backoff_int = 255

        if cur_n != 1 and backoff == -99.0: # does not exist
            backoff_int = -1

        if cur_n == 1:
            f_out_index.write(struct.pack('=BB', score_int, backoff_int))

        else: # cur_n != 1
            is_full = ngram_block.AddNgram(token_id, context_id,
                                           score_int, backoff_int)
            if is_full:
                output_data(f_out_index, f_out_data, ngram_block)

    output_data(f_out_index, f_out_data, ngram_block)
    f_in.close()
    f_out_index.close()
    f_out_data.close()
    print 'Done.'

    if verification_filename:
        DecodeFiles(output_filename_prefix, verification_filename)

if __name__ == '__main__':
    main()
