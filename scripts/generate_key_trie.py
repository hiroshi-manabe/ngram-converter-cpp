#!/usr/bin/python
# -*- coding: utf-8 -*-

# built-ins
import codecs
import collections
import getopt
import os
import re
import sys
import urllib

#externals
import marisa

EXT_KEY = '.key'
EXT_WORD = '.word'
EXT_KEY_WORD = '.key_word'

def Usage():
    print sys.argv[0] + ' - generate the trie for N-gram keys.'
    print 'Usage: '  + sys.argv[0] + \
        ' -k <key-word filename>' + \
        ' -w <word trie filename>' + \
        ' -o <output filename prefix> '
    sys.exit()

def to_id(trie, agent, query_key):
    agent.set_query(query_key)
    if not trie.lookup(agent):
        raise ValueError(query_key)
    return agent.key_id()

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hk:w:o:')
    except getopt.GetoptError:
        Usage()
        sys.exit(2)

    key_word_filename = ''
    word_trie_filename = ''
    output_filename_prefix = ''

    for k, v in opts:
        if k == '-h':
            usage()
            sys.exit()
        elif k == '-k':
            key_word_filename = v
        elif k == '-w':
            word_trie_filename = v
        elif k == '-o':
            output_filename_prefix = v

    if (key_word_filename == '' or
        word_trie_filename == '' or 
        output_filename_prefix == ''):
        Usage()

    f_in = codecs.open(key_word_filename, 'r', 'utf-8')

    keyset_key = marisa.Keyset()
    key_word_dict = collections.defaultdict(list)

    trie_word = marisa.Trie()
    trie_word.load(word_trie_filename)
    agent = marisa.Agent()

    for line in f_in:
        line = line.rstrip('\n')
        if not line:
            continue

        fields = line.split('\t')
        if len(fields) != 2:
            raise ValueError('Number fields not equal to 2')

        keyset_key.push_back(fields[0].encode('utf-8'))

        key_word_dict[fields[0]].append(to_id(trie_word, agent,
                                              fields[1].encode('utf-8')))

    f_in.close()

    trie_key = marisa.Trie()
    trie_key.build(keyset_key)
    trie_key.save(output_filename_prefix + EXT_KEY)

    key_word_list = [None] * len(key_word_dict)

    for key in key_word_dict:
        key_word_list[to_id(trie_key, agent, key.encode('utf-8'))] = key_word_dict[key]

    f_out_key_word = codecs.open(output_filename_prefix + EXT_KEY_WORD, 'w', 'utf-8')
    for v in key_word_list:
        f_out_key_word.write(','.join((str(x) for x in v)))
        f_out_key_word.write('\n')

    f_out_key_word.close()
    print 'Done.'

if __name__ == '__main__':
    main()
