#!/usr/bin/python
# -*- coding: utf-8 -*-

# built-ins
import codecs
import getopt
import os
import re
import struct
import sys

#externals
import marisa

EXT_KEY = '.key'
EXT_PAIR = '.pair'
EXT_VERB_KEY = '.verbkey'
EXT_VERB_DATA = '.verbdata'
PAIR_SEPARATOR = '/'
INFLECTION_START_ORD = ord('0')

def Usage():
    print sys.argv[0] + ' - generate a verb dictionary.'
    print 'Usage: '  + sys.argv[0] + \
        ' -d <dictionary filename prefix> ' \
        ' -v <verb list filename> '
    sys.exit()

def to_id(trie, agent, query_key):
    agent.set_query(query_key)
    if not trie.lookup(agent):
        print query_key
        raise KeyError

    return agent.key_id()

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hd:v:')
    except getopt.GetoptError:
        Usage()
        sys.exit(2)

    dictionary_filename_prefix = ''
    verb_list_filename = ''

    for k, v in opts:
        if k == '-h':
            usage()
            sys.exit()
        elif k == '-d':
            dictionary_filename_prefix = v
        elif k == '-v':
            verb_list_filename = v

    if (not dictionary_filename_prefix or not verb_list_filename):
        Usage()

    trie_pair = marisa.Trie()
    trie_pair.load(dictionary_filename_prefix + EXT_PAIR)
    agent = marisa.Agent()

    f_in = codecs.open(verb_list_filename, 'r', 'utf-8')

    key_id_dict = {}
    keyset = marisa.Keyset()
    num_data = 0

    for line in f_in:
        line = line.rstrip('\n')
        fields = line.split('\t')

        codes = [(ord(x[-1]) - INFLECTION_START_ORD) if x[-2] == '/' else 0
                 for x in fields[1:]]

        for i in range(2, len(fields)):
            if fields[i][-2] == '/':
                fields[i] = fields[i][:-2]
            
        field_ids = [to_id(trie_pair, agent, x.encode('utf-8')) for x
                     in fields[1:]]

        for i in range(len(field_ids)):
            field_ids[i] |= (codes[i] << 24)

        if len(field_ids) == 2:
            field_ids[1:1] = [0xFFFFFFFF]

        key_id_dict.setdefault(fields[0], []).append(tuple(field_ids))
        num_data += 1
        keyset.push_back(fields[0].encode('utf-8'))
        
    trie_verb = marisa.Trie()
    trie_verb.build(keyset)
    trie_verb.save(dictionary_filename_prefix + EXT_VERB_KEY)

    num_keys = trie_verb.num_keys()
    f_out = open(dictionary_filename_prefix + EXT_VERB_DATA, 'wb')
    f_out.write(struct.pack('=LL', num_keys + 1, num_data)) # + 1: sentinel

    pos = 0

    for i in range(num_keys):
        f_out.write(struct.pack('=L', pos))
        agent.set_query(i)
        trie_verb.reverse_lookup(agent)
        key = agent.key_str().decode('utf-8')

        for id in key_id_dict[key]:
            pos += 1

    f_out.write(struct.pack('=L', pos)) # sentinel

    num_data_written = 0
    for i in range(num_keys):
        agent.set_query(i)
        trie_verb.reverse_lookup(agent)
        key = agent.key_str().decode('utf-8')
        for id in key_id_dict[key]:
            f_out.write(struct.pack('=LLL', id[0], id[1], id[2]))
            num_data_written += 1

    assert(num_data == num_data_written)

    f_in.close()
    f_out.close()
    print 'Done.'

if __name__ == '__main__':
    main()
