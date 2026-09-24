"""Checks a converted mod for internal consistency against EU5's own definitions.

Usage: python tools/validate_output.py <converted mod folder> [<EU5 install folder>]

Checks every file is valid UTF-8 with balanced braces, that every country tag, character and
dynasty the mod refers to is defined, that every localisation key it uses exists in every language,
and that each ruler trait passes the allow rules in EU5's in_game/common/traits/00_ruler.txt.
Prints the problems found and exits non-zero if there are any.
"""
import re, os, sys, glob
mod=sys.argv[1].rstrip("/\\")
V=os.path.join(sys.argv[2] if len(sys.argv)>2 else r"C:/Program Files (x86)/Steam/steamapps/common/Europa Universalis V", "game")
S=mod+'/main_menu/setup/start/'
def read(p):
    b=open(p,'rb').read()
    try: t=b.decode('utf-8')
    except UnicodeDecodeError as e: print('INVALID UTF-8',p,e.start); t=b.decode('utf-8','replace')
    return t.lstrip('\ufeff')
def nocomment(t): return re.sub(r'#[^\n]*','',t)
problems=0
def bad(*a):
    global problems; problems+=1
    if problems<=40: print(' ',*a)
files={p.replace(chr(92),'/'):read(p) for p in glob.glob(mod+'/**/*.*',recursive=True) if p.endswith(('.txt','.yml'))}
for p,t in files.items():
    if p.endswith('.txt'):
        c=nocomment(t)
        if c.count('{')!=c.count('}'): bad('braces',os.path.basename(p),c.count('{'),c.count('}'))
countries=nocomment(files[S+'10_countries.txt'])
tags=set(re.findall(r'^\s{1,2}([A-Z][A-Z0-9]{2})\s*=\s*\{',countries,re.M))
print(len(tags),'countries defined')
# tags referenced
for f,pat in [('12_diplomacy.txt',r'\b(?:first|second)\s*=\s*([A-Z][A-Z0-9]{2})\b'),('16_wars.txt',r'\b(?:country|caller|attacker|defender)\s*=\s*([A-Z][A-Z0-9]{2})\b'),
              ('16_wars.txt',r'name\s*=\s*"([A-Z][A-Z0-9]{2})"'),('27_armies.txt',r'\bcountry\s*=\s*([A-Z][A-Z0-9]{2})\b'),
              ('07_cities_and_buildings.txt',r'\btag\s*=\s*([A-Z][A-Z0-9]{2})\b'),('13_religion.txt',r'\b(?:tag|country)\s*=\s*([A-Z][A-Z0-9]{2})\b'),
              ('15_international_organizations.txt',r'\bleader\s*=\s*([A-Z][A-Z0-9]{2})\b'),('05_characters.txt',r'\btag\s*=\s*([A-Z][A-Z0-9]{2})\b')]:
    if S+f not in files: continue
    for t in set(re.findall(pat,nocomment(files[S+f]))):
        if t not in tags: bad('undefined tag',t,'in',f)
# characters
chars_txt=nocomment(files[S+'05_characters.txt'])
chars=set(re.findall(r'^\t([a-z][a-z0-9_]*)\s*=\s*\{',chars_txt,re.M))
print(len(chars),'characters defined')
for ref in set(re.findall(r'\b(?:ruler|heir|character|regent|consort)\s*=\s*([a-z][a-z0-9_]*)',countries)):
    if ref not in chars and ref!='random': bad('undefined character in 10_countries',ref)
for ref in set(re.findall(r'\b(?:father|mother|spouse)\s*=\s*([a-z][a-z0-9_]*)',chars_txt)):
    if ref not in chars: bad('undefined family member',ref)
for f in ['11_art.txt','13_religion.txt','15_international_organizations.txt','27_armies.txt']:
    if S+f in files:
        for ref in set(re.findall(r'\b(?:character|artist|leader)\s*=\s*([a-z][a-z0-9_]*)',nocomment(files[S+f]))):
            if ref not in chars: bad('undefined character',ref,'in',f)
# dynasties
dyn=nocomment(files[S+'04_dynasties.txt'])
dyns=set(re.findall(r'^\s*([a-z][a-z0-9_]*)\s*=\s*\{\s*name',dyn,re.M))
for ref in set(re.findall(r'\bdynasty\s*=\s*([a-z][a-z0-9_]*)',chars_txt)):
    if ref not in dyns: bad('undefined dynasty',ref)
# localisation keys used by the mod
loc={}
for p,t in files.items():
    if p.endswith('.yml'):
        lang=os.path.basename(os.path.dirname(p)); loc.setdefault(lang,set()).update(re.findall(r'^ ([A-Za-z0-9_]+):',t,re.M))
# Loc keys only: character ids like ck3_war_ruler (the ruler of tag WAR) are not localised.
used=set(k for k in re.findall(r'\b(ck3_(?:name|nick|house|war)_[a-z0-9_]+)',chars_txt+nocomment(files[S+'16_wars.txt'])+dyn) if k not in chars)
conv_tags={t for t in re.findall(r'^\t\t([A-Z][A-Z0-9]{2}) = \{ #',files[S+'10_countries.txt'],re.M)}
for lang,keys in sorted(loc.items()):
    miss=[k for k in used if k not in keys]; misst=[t for t in conv_tags if t not in keys]
    if miss or misst: bad(lang,'missing loc keys',len(miss),miss[:3],'tags',len(misst),misst[:3])
print('localisation languages:',len(loc))

# ruler traits
src=open(V+"/in_game/common/traits/00_ruler.txt",encoding='utf-8-sig').read()
src=re.sub(r'#[^\n]*','',src)
def block(t,start):
    d=1; i=start
    while d: d+={'{':1,'}':-1}.get(t[i],0); i+=1
    return t[start:i-1]
rules={}
for m in re.finditer(r'^(\w+)\s*=\s*\{',src,re.M):
    body=block(src,m.end()); a=re.search(r'\ballow\s*=\s*\{',body)
    rules[m.group(1)]=block(body,a.end()) if a else ''
out=chars_txt
n=0
for blk in re.finditer(r'^\t(ck3_\w+) = \{(.*?)^\t\}',out,re.M|re.S):
    body=blk.group(2); ab=dict(re.findall(r'\b(adm|dip|mil) = (\d+)',body)); traits=re.findall(r'ruler_trait = (\w+)',body)
    for t in traits:
        n+=1; allow=rules.get(t)
        if allow is None: bad('unknown trait',t); continue
        for stat,op,val in re.findall(r'\b(adm|dip|mil|ADM|DIP|MIL)\s*(>=|<=|>|<)\s*(\d+)',allow):
            if not eval(f"{int(ab[stat.lower()])}{op}{val}"): bad(blk.group(1),t,stat,op,val)
        for ex in re.findall(r'NOT\s*=\s*\{\s*has_trait\s*=\s*(\w+)\s*\}',allow):
            if ex in traits: bad(blk.group(1),t,'excludes',ex)
print(n,'ruler traits checked')

print('PROBLEMS:',problems)
sys.exit(1 if problems else 0)
