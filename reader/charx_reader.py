#!/usr/bin/python3
"""Bounded CHARX asset resolver. No extraction, network access, or third-party modules."""
import base64
import binascii
import json
import os
import stat
import sys
import zipfile

MAX_ARCHIVE = 256 * 1024 * 1024
MAX_JSON = 4 * 1024 * 1024
MAX_IMAGE = 24 * 1024 * 1024
MAX_ENTRIES = 4096


class CardError(ValueError):
    pass


def safe_path(path):
    return (bool(path) and len(path) <= 1024 and not path.startswith('/')
            and '\\' not in path and '\x00' not in path
            and all(part not in ('', '.', '..') for part in path.split('/')))


def read_entry(archive, entries, path, limit):
    if not safe_path(path) or path not in entries:
        raise CardError('Missing or unsafe asset path')
    info = entries[path]
    mode = info.external_attr >> 16
    if info.flag_bits & 1 or stat.S_ISLNK(mode) or info.is_dir():
        raise CardError('Encrypted or nonregular entry')
    if info.file_size > limit:
        raise CardError('Entry exceeds size limit')
    with archive.open(info) as stream:
        data = stream.read(limit + 1)
    if len(data) > limit:
        raise CardError('Entry exceeds size limit')
    return data


def resolve_uri(uri, archive, entries):
    if uri.startswith('embeded://'):
        return read_entry(archive, entries, uri[len('embeded://'):], MAX_IMAGE)
    if uri == 'ccdefault:':
        # CHARX has no mandated default. Use only these documented conventional paths.
        for path in ('assets/icon/images/main.png', 'assets/icon/images/main.webp',
                     'assets/icon/images/main.jpg'):
            if path in entries:
                return read_entry(archive, entries, path, MAX_IMAGE)
        raise CardError('No default icon')
    if uri.startswith('data:image/'):
        header, sep, payload = uri.partition(',')
        if not sep or not header.endswith(';base64') or len(payload) > (MAX_IMAGE + 2) // 3 * 4:
            raise CardError('Unsupported or oversized data URI')
        try:
            data = base64.b64decode(payload, validate=True)
        except binascii.Error as exc:
            raise CardError('Invalid base64') from exc
        if len(data) > MAX_IMAGE:
            raise CardError('Image exceeds size limit')
        return data
    raise CardError('Only embedded and base64 image assets are supported')


def read_icon(path):
    # Open without waiting for a FIFO writer, then validate the opened file.
    with open(path, 'rb', opener=lambda name, flags: os.open(name, flags | os.O_NONBLOCK)) as source:
        if not stat.S_ISREG(os.fstat(source.fileno()).st_mode):
            raise CardError('Input must be a regular file')
        source.seek(0, 2)
        if source.tell() > MAX_ARCHIVE:
            raise CardError('Archive exceeds size limit')
        source.seek(0)
        with zipfile.ZipFile(source) as archive:
            infos = archive.infolist()
            if len(infos) > MAX_ENTRIES:
                raise CardError('Too many entries')
            entries = {info.filename: info for info in infos}
            if len(entries) != len(infos):
                raise CardError('Duplicate archive paths')
            card = json.loads(read_entry(archive, entries, 'card.json', MAX_JSON))
            if not isinstance(card, dict) or card.get('spec') != 'chara_card_v3':
                raise CardError('Not a Character Card V3')
            data = card.get('data')
            if not isinstance(data, dict):
                raise CardError('Missing card data')
            assets = data.get('assets', [{'type': 'icon', 'name': 'main', 'uri': 'ccdefault:'}])
            if not isinstance(assets, list):
                raise CardError('Invalid assets')
            icons = [a for a in assets if isinstance(a, dict) and a.get('type') == 'icon']
            main = [a for a in icons if a.get('name') == 'main']
            # Never substitute a gallery/emotion or secondary icon for an explicit main icon.
            selected = main[0] if main else next(iter(icons), None)
            if selected is None or not isinstance(selected.get('uri'), str):
                raise CardError('No usable icon')
            return resolve_uri(selected['uri'], archive, entries)


def main():
    if len(sys.argv) != 2:
        return 2
    try:
        # Additional process limits; the parent also enforces a wall-clock deadline.
        import resource
        resource.setrlimit(resource.RLIMIT_AS, (256 * 1024 * 1024,) * 2)
        resource.setrlimit(resource.RLIMIT_CPU, (5, 5))
        sys.stdout.buffer.write(read_icon(sys.argv[1]))
        return 0
    except (OSError, ValueError, zipfile.BadZipFile, RuntimeError, NotImplementedError,
            MemoryError, RecursionError) as exc:
        print(str(exc), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
