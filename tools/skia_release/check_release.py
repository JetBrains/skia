#! /usr/bin/env python3

import json
import sys
import urllib.error
import urllib.request

import common


def main():
  headers = common.github_headers()
  version = common.version()
  build_type = common.build_type()
  target = common.target()
  machine = common.machine()
  classifier = common.classifier()

  try:
    resp = urllib.request.urlopen(
        urllib.request.Request(
            'https://api.github.com/repos/' + common.github_repo() + '/releases/tags/' + version,
            headers=headers,
        )
    ).read()
    artifacts = [x['name'] for x in json.loads(resp.decode('utf-8'))['assets']]
    zip_name = 'Skia-' + version + '-' + target + '-' + build_type + '-' + machine + classifier + '.zip'
    if zip_name in artifacts:
      print('> Artifact "' + zip_name + '" exists, stopping')
      return 1
    return 0
  except urllib.error.URLError:
    return 0


if __name__ == '__main__':
  sys.exit(main())
