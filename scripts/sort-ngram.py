#!/usr/bin/python
# -*- coding: utf-8 -*-

# built-ins
import codecs
import getopt
import os
import re
import struct
import sys

# externals
import marisa

# personals
import util

EXT_KEY = '.key'
EXT_PAIR = '.pair'
EXT_TEXT = '.lmtext'
EXT_TEKEI = '.tekei'
ELEM_SEPARATOR = '/'
INFLECTION_START = ord('0')
INVALID_SCORE = -99

def CreateTranslateMap(src, dst):
    return dict((ord(s), ord(d)) for (s,d) in zip(src, dst))

HIRA = reduce(lambda a, b: a+b,
              (unichr(x) for x in range(ord(u'ぁ'), ord(u'ん') + 1)))
KATA = reduce(lambda a, b: a+b,
              (unichr(x) for x in range(ord(u'ァ'), ord(u'ン') + 1)))
HIRA_TO_KATA_MAP = CreateTranslateMap(HIRA, KATA)
KATA_TO_HIRA_MAP = CreateTranslateMap(KATA, HIRA)
TEKEI_TO_DAKU_MAP = CreateTranslateMap(u'タチテト', u'ダジデド')
TEKEI_DAKU_SET = set(u'だじでど')

def Usage():
    print sys.argv[0] + ' - sort SRILM format ngram file.'
    print 'Usage: '  + sys.argv[0] + \
        ' -i <input filename>' + \
        ' -o <output filename prefix> ' + \
        ' -f <inflection table filename>' + \
        ' -t <tekei filename>'
    sys.exit()

def to_id(trie, agent, query_key):
    agent.set_query(query_key)
    trie.lookup(agent)
    return agent.key_id()

def main():
    def output_ngram(f_out, n, context_dict, context_id_dicts):
        f_out.write('#%d\t%d\n' % (n, len(context_dict)))

        for context_key in sorted(context_dict.keys()):
            context_id_dicts[n][context_key] = len(context_id_dicts[n])
            first = context_key[0]
            if len(context_key) > 1:
                next_context_id = context_id_dicts[n-1][tuple(context_key[1:])]
            else:
                next_context_id = 0

            f_out.write('%d\t' % (context_id_dicts[n][context_key]) +
                        '%d\t%d\t' % (context_key[0], next_context_id) +
                        '%f\t%f\t%s\n' % context_dict[context_key])

        return

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hi:o:t:f:')
    except getopt.GetoptError:
        Usage()
        sys.exit(2)

    input_filename = ''
    output_filename_prefix = ''
    inflection_filename = ''
    tekei_filename = ''

    for k, v in opts:
        if k == '-h':
            usage()
            sys.exit()
        elif k == '-i':
            input_filename = v
        elif k == '-o':
            output_filename_prefix = v
        elif k == '-f':
            inflection_filename = v
        elif k == '-t':
            tekei_filename = v

    if (input_filename == '' or output_filename_prefix == '' or
        inflection_filename == '' or tekei_filename == ''):
        Usage()

    f_in = codecs.open(input_filename, 'r', 'utf-8')
    f_out_text = codecs.open(output_filename_prefix + EXT_TEXT, 'w', 'utf-8')
    temp_set = util.ReadFileToTupleSet(tekei_filename, 2)
    tekei_set = set((x[1].translate(HIRA_TO_KATA_MAP), x[0]) for x in temp_set
                    if x[1][0] not in TEKEI_DAKU_SET)
    inflection_table = util.ReadFileToDict(inflection_filename, 1, 1)

    cur_n = 0
    pair_id = 1
    pair_dict = {}
    context_dict = {}
    context_id_dicts = [{}]
    keyset_key = marisa.Keyset()
    keyset_pair = marisa.Keyset()
    trie_key = marisa.Trie()
    trie_pair = marisa.Trie()
    agent = marisa.Agent()
    min_score = 99.0
    max_backoff = -99.0
    tekei_dict = {}

    for line in f_in:
        line = line.rstrip('\n')
        if not line:
            continue

        if line[0] == '\\':
            m = re.search(r'^\\(\d+)-grams:', line)
            if (cur_n and (m or re.search(r'^\\end\\', line))):
                if  cur_n == 1:
                    trie_key.build(keyset_key)
                    trie_key.save(output_filename_prefix + EXT_KEY)
                    trie_pair.build(keyset_pair)
                    trie_pair.save(output_filename_prefix + EXT_PAIR)
                    f_out_tekei = open(output_filename_prefix + EXT_TEKEI, 'wb')
                    sei_code = (ord(inflection_table[u'連用形-テ形-清']) -
                                INFLECTION_START)
                    daku_code = (ord(inflection_table[u'連用形-テ形-濁']) -
                                 INFLECTION_START)

                    f_out_tekei.write(struct.pack('BB', sei_code, daku_code))
                    for k, v in pair_dict.iteritems():
                        id = to_id(trie_pair, agent, k)
                        context_dict[(id,)] = v
                        if k in tekei_dict:
                            orig = tekei_dict[k]
                            orig_id = 0xffffffff
                            if orig:
                                orig_id = to_id(trie_pair, agent, orig)
                            f_out_tekei.write(struct.pack('=LL', id, orig_id))
                    f_out_tekei.close()

                output_ngram(f_out_text, cur_n, context_dict, context_id_dicts)

            if m:
                cur_n = int(m.group(1))
                context_dict = {}
                context_id_dicts.append({})
                print 'Processing %d-gram...' % cur_n

            continue

        if cur_n == 0:
            continue

        fields = line.split('\t')
        if len(fields) < 2:
            continue

        if len(fields) == 2:
            fields.append('0.0')

        score = float(fields[0])
        backoff = float(fields[2])
        if score < min_score and score > -99:
            min_score = score
        if backoff > max_backoff:
            max_backoff = backoff

        if cur_n == 1:
            k = fields[1].encode('utf-8')
            keyset_pair.push_back(k)
            elems = fields[1].split(ELEM_SEPARATOR)

            if k[0] != '<' and len(elems) > 1 and not elems[-1]:
                keyset_key.push_back(elems[0].encode('utf-8'))

            pair_dict[k] = (float(fields[0]), float(fields[2]), fields[1])
            if (len(elems) > 2 and
                (elems[0], elems[1]) in tekei_set and elems[2][:3] in (
                    u'助動詞', u'助詞-')):
                elems[0] = elems[0].translate(TEKEI_TO_DAKU_MAP)
                
                daku_key = ELEM_SEPARATOR.join(elems)
                daku_key_encoded = daku_key.encode('utf-8')
                keyset_pair.push_back(daku_key_encoded)

                if not elems[-1]:
                    keyset_key.push_back(elems[0].encode('utf-8'))

                pair_dict[daku_key_encoded] = (INVALID_SCORE, INVALID_SCORE,
                                               daku_key)
                tekei_dict[k] = ''
                tekei_dict[daku_key_encoded] = k

        else:
            ngram = [to_id(trie_pair, agent, x.encode('utf-8')) for x
                     in reversed(fields[1].split(' '))]
            context_dict[tuple(ngram)] = (float(fields[0]), float(fields[2]), fields[1])

    f_in.close()
    f_out_text.close()
    print 'Done.'
    print 'min_score = %f, max_backoff = %f' % (min_score, max_backoff)


if __name__ == '__main__':
    main()
