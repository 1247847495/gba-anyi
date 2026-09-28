# -*- coding: utf-8 -*-
out = []
s = u'安易GBA仿真器'
b = s.encode('shift_jis')
out.append(u'bytes: ' + ' '.join('%02X' % x for x in b) + ' len %d' % len(b))
for ch in u'安易仿真器模拟戏拟':
    try:
        e = ch.encode('shift_jis')
        out.append(ch + u' %02X %02X' % (e[0], e[1]))
    except Exception:
        out.append(ch + u' NOT-IN-JIS')
open(r'D:\GBA\build\title_sjis_out.txt', 'w', encoding='utf-8').write(u'\n'.join(out))
