import sys
import re
import subprocess
import xml.etree.ElementTree as ET

upstream_filename = sys.argv[-2]   # dest
translated_filename = sys.argv[-1] # src

translated_tree = ET.parse(translated_filename)
troot = translated_tree.getroot()

translation_dict = {}

for contextE in troot.iter('context'):
    context = contextE.find('name').text
    for messageE in contextE.iter('message'):
        source = messageE.find('source').text
        translation = messageE.find('translation').text
        if source and translation:
            translation_dict[(context, source.strip())] = translation

del translated_tree

upstream_tree = ET.parse(upstream_filename)
uroot = upstream_tree.getroot()

for contextE in uroot.iter('context'):
    context = contextE.find('name').text
    for messageE in contextE.iter('message'):
        source = messageE.find('source').text
        if source:
            translation = translation_dict.get((context, source.strip()))
            if translation:
                tE = messageE.find('translation')
                tE.text = translation
                del tE.attrib['type']

upstream_tree.write('{}.generated'.format(upstream_filename), xml_declaration=True, encoding='utf-8', short_empty_elements=False)

# n = int(subprocess.run("cat {} | grep '<source>' | wc -l".format(translated_filename), capture_output=True, shell=True).stdout)
