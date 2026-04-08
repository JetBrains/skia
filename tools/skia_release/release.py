#! /usr/bin/env python3

import json
import os
import re
import sys
import urllib.error
import urllib.request

import common


def main():
  version = common.version()
  build_type = common.build_type()
  machine = common.machine()
  target = common.target()
  classifier = common.classifier()

  zip_name = 'Skia-' + version + '-' + target + '-' + build_type + '-' + machine + classifier + '.zip'
  zip_path = common.skia_dir() / zip_name
  if not zip_path.exists():
    print('Can\'t find "' + zip_name + '"')
    return 1

  headers = common.github_headers()
  releases_url = 'https://api.github.com/repos/' + common.github_repo() + '/releases'

  try:
    resp = urllib.request.urlopen(
        urllib.request.Request(releases_url + '/tags/' + version, headers=headers)
    ).read()
  except urllib.error.URLError:
    data = json.dumps({
        'tag_name': version,
        'name': version,
        'target_commitish': common.current_revision(),
    })
    resp = urllib.request.urlopen(
        urllib.request.Request(releases_url, data=data.encode('utf-8'), headers=headers)
    ).read()

  upload_url = re.match(
      'https://.*/assets',
      json.loads(resp.decode('utf-8'))['upload_url'],
  ).group(0)

  print('Uploading', zip_name, 'to', upload_url)
  headers['Content-Type'] = 'application/zip'
  headers['Content-Length'] = os.path.getsize(zip_path)
  with zip_path.open('rb') as data:
    urllib.request.urlopen(
        urllib.request.Request(upload_url + '?name=' + zip_name, data=data, headers=headers)
    )

  return 0


if __name__ == '__main__':
  sys.exit(main())
