#!/usr/bin/env python3
"""Shared Plus ID ban lists: the maintainer's tool (docs/architecture/plus-id.md).

    banlist.py keygen OUT          OUT.key (private PEM) and OUT.pub (hex, for sv_banLists)
    banlist.py sign LIST KEY       LIST.sig: ECDSA P-256 over SHA-256 of LIST, r||s in base64
    banlist.py verify LIST PUBHEX  exit 0 when LIST.sig is good for PUBHEX, else 1
    banlist.py check LIST          exit 0 when LIST has the format and limits servers take

Uses the openssl CLI. The checks are the server's (src/libcod/plusid_lists.cpp).
"""
import base64, calendar, hashlib, json, os, re, subprocess, sys, tempfile, time

MAX_BYTES = 4 * 1024 * 1024
MAX_ENTRIES = 100000
# a compressed P-256 key is this SubjectPublicKeyInfo prefix and the 33 bytes
SPKI_PREFIX = bytes.fromhex('3039301306072a8648ce3d020106082a8648ce3d030107032200')
# the longest text of each field, in UTF-8 bytes (the sizes of PlusIdBan less one)
TEXT = {'reason': 127, 'by': 63, 'evidence': 255}


def read(path, mode='rb'):
    with open(path, mode) as f:
        return f.read()


def openssl(*args, data=None):
    return subprocess.run(['openssl'] + list(args), input=data, capture_output=True, check=True).stdout


def der_int(der, i):
    if der[i] != 2:
        raise ValueError('not a DER signature')
    n = der[i + 1]
    return int.from_bytes(der[i + 2:i + 2 + n], 'big'), i + 2 + n


def der_to_rs(der):
    r, i = der_int(der, 2)
    s, _ = der_int(der, i)
    return r.to_bytes(32, 'big') + s.to_bytes(32, 'big')


def rs_to_der(rs):
    def integer(b):
        b = b.lstrip(b'\0') or b'\0'
        if b[0] & 0x80:
            b = b'\0' + b
        return b'\x02' + bytes([len(b)]) + b
    body = integer(rs[:32]) + integer(rs[32:])
    return b'\x30' + bytes([len(body)]) + body


def keygen(out):
    openssl('genpkey', '-algorithm', 'EC', '-pkeyopt', 'ec_paramgen_curve:P-256', '-out', out + '.key')
    os.chmod(out + '.key', 0o600)
    spki = openssl('pkey', '-in', out + '.key', '-pubout', '-outform', 'DER', '-ec_conv_form', 'compressed')
    with open(out + '.pub', 'w') as f:
        f.write(spki[-33:].hex() + '\n')
    print(f'wrote {out}.key (keep it secret) and {out}.pub')
    return 0


def sign(path, key):
    der = openssl('dgst', '-sha256', '-sign', key, data=read(path))
    with open(path + '.sig', 'w') as f:
        f.write(base64.b64encode(der_to_rs(der)).decode() + '\n')
    print(f'wrote {path}.sig')
    return 0


def verify(path, pubhex):
    try:
        pk = bytes.fromhex(pubhex)
        rs = base64.b64decode(read(path + '.sig').strip(), validate=True)
        if len(pk) != 33 or len(rs) != 64:
            raise ValueError
    except (ValueError, OSError):
        print('bad key or signature file')
        return 1
    with tempfile.TemporaryDirectory(prefix='banlist_') as tmp:
        files = {'pub.der': SPKI_PREFIX + pk, 'sig.der': rs_to_der(rs), 'digest': hashlib.sha256(read(path)).digest()}
        for name, data in files.items():
            with open(os.path.join(tmp, name), 'wb') as f:
                f.write(data)
        r = subprocess.run(['openssl', 'pkeyutl', '-verify', '-pubin', '-keyform', 'DER', '-inkey', os.path.join(tmp, 'pub.der'),
                            '-in', os.path.join(tmp, 'digest'), '-sigfile', os.path.join(tmp, 'sig.der')], capture_output=True)
    print('signature ok' if r.returncode == 0 else 'bad signature')
    return 0 if r.returncode == 0 else 1


def valid_id(text):
    return isinstance(text, str) and re.fullmatch(r'P-[A-Z2-7]{4}-[A-Z2-7]{4}-[A-Z2-7]{4}-[A-Z2-7]{4}', text) is not None


def parse_time(text):
    """PlusId_ParseTime: "YYYY-MM-DD" or "YYYY-MM-DDTHH:MM:SSZ", a real date
    from 1970, as Unix seconds; None for anything else."""
    for fmt, n in (('%Y-%m-%d', 10), ('%Y-%m-%dT%H:%M:%SZ', 20)):
        if isinstance(text, str) and len(text) == n:
            try:
                t = time.strptime(text, fmt)
            except ValueError:
                return None
            seconds = calendar.timegm(t)
            return seconds if t.tm_year >= 1970 and time.strftime(fmt, time.gmtime(seconds)) == text else None
    return None


def valid_time(text):
    return parse_time(text) is not None


def text_ok(value, longest):
    return isinstance(value, str) and len(value.encode()) <= longest


def problems(path):
    """What keeps a server from taking the list, empty when nothing does."""
    if os.path.getsize(path) > MAX_BYTES:
        return [f'over {MAX_BYTES} bytes']
    try:
        doc = json.loads(read(path))
    except ValueError as e:
        return [f'not JSON: {e}']
    if not isinstance(doc, dict):
        return ['not an object']
    out = []
    if doc.get('format') != 1 or isinstance(doc.get('format'), bool):
        out.append('format is not 1')
    if not text_ok(doc.get('name'), 63) or not doc.get('name'):
        out.append('name missing or over 63 bytes')
    if 'appeal' in doc and doc['appeal'] is not None and not text_ok(doc['appeal'], 255):
        out.append('appeal over 255 bytes')
    if not valid_time(doc.get('updated')):
        out.append('updated is not YYYY-MM-DD or YYYY-MM-DDTHH:MM:SSZ')
    elif parse_time(doc['updated']) > time.time() + 86400:
        # servers refuse a version older than the one they hold: a typo here freezes the list
        out.append('updated is more than a day in the future')
    entries = doc.get('entries')
    if not isinstance(entries, list):
        return out + ['entries is not a list']
    if len(entries) > MAX_ENTRIES:
        return out + [f'over {MAX_ENTRIES} entries']
    for n, e in enumerate(entries):
        if not isinstance(e, dict):
            out.append(f'entry {n}: not an object')
            continue
        if not valid_id(e.get('id')):
            out.append(f'entry {n}: id is not P-XXXX-XXXX-XXXX-XXXX')
        for key in ('guid', 'ip'):
            if key in e:
                out.append(f'entry {n}: {key} is never published in a shared list')
        for key, longest in TEXT.items():
            if key in e and not text_ok(e[key], longest):
                out.append(f'entry {n}: {key} is not text of at most {longest} bytes')
        for key in ('date', 'expires'):
            if e.get(key) is not None and not valid_time(e[key]):
                out.append(f'entry {n}: {key} is not a date')
        # PlusId_ParseBan: an expiry must come after the date (absent is 1970-01-01)
        if valid_time(e.get('expires')) and (e.get('date') is None or valid_time(e['date'])) \
                and parse_time(e['expires']) <= (parse_time(e['date']) if e.get('date') is not None else 0):
            out.append(f'entry {n}: expires is not after date')
    return out


def check(path):
    found = problems(path)
    for p in found[:50]:
        print(p)
    print('ok' if not found else f'{len(found)} problem(s)')
    return 1 if found else 0


def main(argv):
    commands = {'keygen': (keygen, 1), 'sign': (sign, 2), 'verify': (verify, 2), 'check': (check, 1)}
    if len(argv) < 2 or argv[1] not in commands or len(argv) != commands[argv[1]][1] + 2:
        print(__doc__.strip())
        return 2
    return commands[argv[1]][0](*argv[2:])


if __name__ == '__main__':
    sys.exit(main(sys.argv))
