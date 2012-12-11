#!/usr/bin/python
# -*- coding: utf-8 -*-

# built-ins
import codecs
import getopt
import os
import re
import sys

#externals
import marisa

EXT_KEY = '.key'
EXT_PAIR = '.pair'
EXT_TEXT = '.lmtext'
PAIR_SEPARATOR = '/'

def Usage():
    print sys.argv[0] + ' - sort SRILM format ngram file.'
    print 'Usage: '  + sys.argv[0] + \
        ' -i <input filename>' + \
        ' -o <output filename prefix> '
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
        opts, args = getopt.getopt(sys.argv[1:], 'hi:o:t:')
    except getopt.GetoptError:
        Usage()
        sys.exit(2)

    input_filename = ''
    output_filename_prefix = ''

    for k, v in opts:
        if k == '-h':
            usage()
            sys.exit()
        elif k == '-i':
            input_filename = v
        elif k == '-o':
            output_filename_prefix = v

    if input_filename == '' or output_filename_prefix == '':
        Usage()

    f_in = codecs.open(input_filename, 'r', 'utf-8')
    f_out_text = codecs.open(output_filename_prefix + EXT_TEXT, 'w', 'utf-8')

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
                    for k, v in pair_dict.iteritems():
                        context_dict[(to_id(trie_pair, agent, k),)] = v

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
            fields.append('-99')

        score = float(fields[0])
        backoff = float(fields[2])
        if score < min_score and score > -99:
            min_score = score
        if backoff > max_backoff:
            max_backoff = backoff

        if cur_n == 1:
            k = fields[1].encode('utf-8')
            keyset_pair.push_back(k)
            pair = k.split(PAIR_SEPARATOR, 1);
            if len(pair) > 1:
                keyset_key.push_back(pair[0])

            pair_dict[k] = (float(fields[0]), float(fields[2]), fields[1])
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
