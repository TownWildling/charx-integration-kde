import base64
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

spec = importlib.util.spec_from_file_location('reader', Path(__file__).parents[1] / 'reader/charx_reader.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)


class ReaderTests(unittest.TestCase):
    def archive(self, assets, files=None, **changes):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        path = Path(temp.name) / 'test.charx'
        card = dict(spec='chara_card_v3', spec_version='3.0', data={'assets': assets})
        card.update(changes)
        with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED) as z:
            z.writestr('card.json', json.dumps(card))
            for name, data in (files or {}).items():
                z.writestr(name, data)
        return path

    @staticmethod
    def icon(uri, name='main'):
        return dict(type='icon', name=name, uri=uri, ext='png')

    def test_main_over_secondary_and_gallery(self):
        path = self.archive([self.icon('embeded://secondary', 'secondary'),
                             dict(type='gallery', uri='embeded://gallery'),
                             self.icon('embeded://portrait')],
                            {'secondary': b'wrong', 'portrait': b'right'})
        self.assertEqual(reader.read_icon(path), b'right')

    def test_future_version_and_unknown_fields(self):
        path = self.archive([self.icon('embeded://a')], {'a': b'ok'},
                            spec_version='3.9', future=True)
        self.assertEqual(reader.read_icon(path), b'ok')

    def test_data_uri(self):
        path = self.archive([self.icon('data:image/png;base64,' + base64.b64encode(b'png').decode())])
        self.assertEqual(reader.read_icon(path), b'png')

    def test_reject_unsafe_or_external_uris(self):
        for uri in ('embeded://../secret', 'embeded:///etc/passwd', 'embeded://a\\b',
                    'https://example.com/a.png', 'file:///etc/passwd',
                    'embedded://a', 'data:image/png;base64,@@@'):
            with self.subTest(uri=uri), self.assertRaises(reader.CardError):
                reader.read_icon(self.archive([self.icon(uri)]))

    def test_explicit_main_does_not_fall_back(self):
        with self.assertRaises(reader.CardError):
            reader.read_icon(self.archive([self.icon('embeded://missing'),
                                           self.icon('embeded://ok', 'other')], {'ok': b'wrong'}))

    def test_default(self):
        path = self.archive([], {'assets/icon/images/main.png': b'default'}, data={})
        self.assertEqual(reader.read_icon(path), b'default')

    def test_invalid_structure(self):
        for change in ({'spec': 'chara_card_v2'}, {'data': []}, {'data': {'assets': {}}}):
            with self.subTest(change=change), self.assertRaises(reader.CardError):
                reader.read_icon(self.archive([], **change))

    def test_bounded_decompression(self):
        old = reader.MAX_IMAGE
        reader.MAX_IMAGE = 10
        self.addCleanup(setattr, reader, 'MAX_IMAGE', old)
        with self.assertRaises(reader.CardError):
            reader.read_icon(self.archive([self.icon('embeded://big')], {'big': b'x' * 1000}))

    def test_duplicate_names(self):
        path = self.archive([])
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter('ignore')
            with zipfile.ZipFile(path, 'a') as z:
                z.writestr('card.json', '{}')
        with self.assertRaises(reader.CardError):
            reader.read_icon(path)

    def test_case_sensitive_paths(self):
        with self.assertRaises(reader.CardError):
            reader.read_icon(self.archive([self.icon('embeded://A.png')], {'a.png': b'image'}))

    def test_symlink(self):
        path = self.archive([self.icon('embeded://link')])
        with zipfile.ZipFile(path, 'a') as z:
            entry = zipfile.ZipInfo('link')
            entry.create_system = 3
            entry.external_attr = 0o120777 << 16
            z.writestr(entry, '/etc/passwd')
        with self.assertRaises(reader.CardError):
            reader.read_icon(path)



if __name__ == '__main__':
    unittest.main()
