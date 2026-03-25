#! /usr/bin/env python3

import argparse
import base64
import os
import platform
import re
import subprocess
from pathlib import Path


def create_parser(version_required=False):
  parser = argparse.ArgumentParser()
  parser.add_argument('--build-type', default='Release')
  parser.add_argument('--enable-ganesh', action=argparse.BooleanOptionalAction, default=True)
  parser.add_argument('--enable-graphite', action=argparse.BooleanOptionalAction, default=False)
  parser.add_argument('--gpu-as-extension', action=argparse.BooleanOptionalAction, default=False)
  parser.add_argument('--version', required=version_required)
  parser.add_argument('--classifier')
  parser.add_argument('--host')
  parser.add_argument('--machine')
  parser.add_argument('--ndk')
  parser.add_argument('--skia-dir')
  parser.add_argument('--target')
  return parser


def repo_root():
  return Path(__file__).resolve().parents[2]


def skia_dir():
  parser = create_parser()
  (args, _) = parser.parse_known_args()
  if args.skia_dir:
    path = Path(args.skia_dir)
    if not path.is_absolute():
      path = repo_root() / path
    return path.resolve()
  return (repo_root() / 'build/skia').resolve()


def host():
  parser = create_parser()
  (args, _) = parser.parse_known_args()
  return args.host if args.host else {
      'Darwin': 'macos',
      'Linux': 'linux',
      'Windows': 'windows',
  }[platform.system()]


def machine():
  parser = create_parser()
  (args, _) = parser.parse_known_args()
  return args.machine if args.machine else {
      'AMD64': 'x64',
      'x86_64': 'x64',
      'arm64': 'arm64',
      'aarch64': 'arm64',
  }[platform.machine()]


def target():
  parser = create_parser()
  (args, _) = parser.parse_known_args()
  return args.target if args.target else host()


def version():
  parser = create_parser()
  args = parser.parse_args()

  if args.version:
    return args.version

  branches = subprocess.check_output(
      ['git', 'branch', '-a', '--contains', 'HEAD'],
      cwd=skia_dir(),
      text=True,
  )
  all_branches = subprocess.check_output(
      ['git', 'branch', '-a'],
      cwd=skia_dir(),
      text=True,
  )
  milestone = infer_milestone(branches, all_branches)

  if milestone is None:
    raise RuntimeError(
        'Unable to infer Skia milestone from branches containing HEAD. '
        'Pass --version explicitly.'
    )

  revision = subprocess.check_output(
      ['git', 'rev-parse', 'HEAD'],
      cwd=skia_dir(),
      text=True,
  ).strip()
  return milestone + '-' + revision[:10]


def infer_milestone(branches, all_branches=''):
  milestone = _find_milestone(branches, containing_head=True)
  if milestone is not None:
    return milestone
  return _find_milestone(all_branches, containing_head=False)


def _find_milestone(branches, containing_head):
  milestone = None
  patterns = [
      r'(?m)^\s*(?:\* )?(?:remotes/[^/]+/)?chrome/(m\d+)$',
      r'(?m)^\s*(?:\* )?(?:remotes/[^/]+/)?skiko-(m\d+)$',
  ]

  for pattern in patterns:
    for match in re.finditer(pattern, branches):
      milestone = match.group(1)
      if containing_head:
        return milestone

  return milestone


def build_type():
  parser = create_parser()
  (args, _) = parser.parse_known_args()
  return args.build_type


def enable_graphite():
  parser = create_parser()
  (args, _) = parser.parse_known_args()
  return args.enable_graphite


def enable_ganesh():
  parser = create_parser()
  (args, _) = parser.parse_known_args()
  return args.enable_ganesh


def gpu_as_extension():
  parser = create_parser()
  (args, _) = parser.parse_known_args()
  return args.gpu_as_extension


def classifier():
  parser = create_parser()
  (args, _) = parser.parse_known_args()
  return '-' + args.classifier if args.classifier else ''


def github_headers():
  basic = os.environ.get('GITHUB_BASIC')
  token = os.environ.get('GITHUB_TOKEN')
  if basic:
    auth = 'Basic ' + base64.b64encode(basic.encode('utf-8')).decode('utf-8')
  elif token:
    auth = 'Bearer ' + token
  else:
    raise RuntimeError('Either GITHUB_BASIC or GITHUB_TOKEN must be set')

  return {
      'Accept': 'application/vnd.github.v3+json',
      'Authorization': auth,
  }


def github_repo():
  repo = os.environ.get('GITHUB_REPOSITORY')
  if repo:
    return repo

  cwd = repo_root()
  remote_name = _current_branch_remote(cwd)
  if remote_name is not None:
    repo = _github_repo_from_remote_name(remote_name, cwd)
    if repo is not None:
      return repo

  repos = _github_repos_from_remotes(cwd)
  if len(repos) == 1:
    return repos[0]
  if len(repos) > 1:
    raise RuntimeError(
        'Unable to infer GitHub repo: multiple GitHub remotes found '
        + f'({", ".join(repos)}). Set GITHUB_REPOSITORY explicitly.'
    )
  raise RuntimeError('Unable to infer GitHub repo: no GitHub remotes found.')


def _current_branch_remote(cwd):
  current_branch = _git_output(cwd, 'rev-parse', '--abbrev-ref', 'HEAD')
  if current_branch == 'HEAD':
    return None

  keys = [
      f'branch.{current_branch}.pushRemote',
      'remote.pushDefault',
      f'branch.{current_branch}.remote',
  ]
  for key in keys:
    try:
      return _git_output(cwd, 'config', '--get', key)
    except subprocess.CalledProcessError:
      pass
  return None


def _github_repo_from_remote_name(remote_name, cwd):
  try:
    remote_url = _git_output(cwd, 'remote', 'get-url', remote_name)
  except subprocess.CalledProcessError:
    return None
  return _github_repo_from_remote_url(remote_url)


def _github_repos_from_remotes(cwd):
  repos = []
  for remote_name in _git_output(cwd, 'remote').splitlines():
    repo = _github_repo_from_remote_name(remote_name, cwd)
    if repo is not None and repo not in repos:
      repos.append(repo)
  return repos


def _git_output(cwd, *args):
  return subprocess.check_output(['git', *args], cwd=cwd, text=True).strip()


def _github_repo_from_remote_url(remote_url):
  match = re.match(
      r'(?:https://(?:[^/@]+@)?github\.com/|git@github\.com:|ssh://git@github\.com/)'
      r'([^/]+/[^/]+?)(?:\.git)?$',
      remote_url,
  )
  if match:
    return match.group(1)
  return None


def ndk():
  parser = create_parser()
  (args, _) = parser.parse_known_args()
  return args.ndk if args.ndk else ''
