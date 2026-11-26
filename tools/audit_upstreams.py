#!/usr/bin/env python3
"""Read public upstream metadata/text at pinned commits. Never downloads game data or runs source."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess
import urllib.parse
import urllib.request

ROOT=Path(__file__).resolve().parents[1]
REPOS=('Marvisak/VIctory','Belobug/VIctory-Belobug-AMD','brcly/TuxBox')
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--gh',type=Path,help='Existing GitHub CLI for public read-only API requests')
args=parser.parse_args()

def api(endpoint):
    assert endpoint.startswith('repos/') and any(endpoint.startswith('repos/'+r+'/') or endpoint=='repos/'+r for r in REPOS)
    if args.gh:
        p=subprocess.run([str(args.gh),'api',endpoint],capture_output=True,text=True,encoding='utf-8',errors='replace')
        if p.returncode:raise RuntimeError('Public GitHub request failed: '+p.stderr[:300])
        return json.loads(p.stdout)
    request=urllib.request.Request('https://api.github.com/'+endpoint,headers={'User-Agent':'VI-Orbis-research'})
    with urllib.request.urlopen(request,timeout=30) as response:return json.load(response)

def source(repo,commit,path):
    url='https://raw.githubusercontent.com/'+repo+'/'+commit+'/'+urllib.parse.quote(path,safe='/')
    with urllib.request.urlopen(url,timeout=30) as response:
        data=response.read(300001)
    if len(data)>300000:raise RuntimeError('Source exceeds inspection bound')
    return data,url

report={'captured_utc':datetime.now(timezone.utc).isoformat(),'method':'Static metadata and selected public text only; not a runtime validation',
        'game_files_downloaded':False,'third_party_source_executed':False,'projects':[]}
for repo in REPOS:
    meta=api('repos/'+repo)
    if meta.get('private'):raise RuntimeError('Only public sources belong in this report')
    branch=meta['default_branch']
    commit=api('repos/'+repo+'/commits/'+urllib.parse.quote(branch,safe=''))['sha']
    tree=api('repos/'+repo+'/git/trees/'+commit+'?recursive=1')
    paths={entry['path'] for entry in tree.get('tree',[]) if entry['type']=='blob'}
    preferred=['README.md','Cargo.toml','docs/architecture.md','docs/ARCHITECTURE.md',
               'docs/PROGRESS.md','crates/victory-loader/Cargo.toml','crates/victory-gpu/Cargo.toml',
               'crates/victory-hle/Cargo.toml','crates/xcl/Cargo.toml','CMakeLists.txt']
    selected=[p for p in preferred if p in paths]
    interesting=sorted(p for p in paths if p.endswith(('Cargo.toml','CMakeLists.txt')))
    selected=list(dict.fromkeys(selected+interesting))[:12]
    dest=ROOT/'research/snapshots'/repo.replace('/','--')/commit
    dest.mkdir(parents=True,exist_ok=True)
    item={'repo':repo,'default_branch':branch,'commit':commit,
          'license':(meta.get('license') or {}).get('spdx_id'),'is_github_fork':meta.get('fork'),
          'tree_was_truncated':tree.get('truncated',False),'files':[]}
    for path in selected:
        data,url=source(repo,commit,path)
        target=dest/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
        text=data.decode('utf-8',errors='replace')
        markers={name:len(re.findall(pattern,text,re.I)) for name,pattern in {
            'windows':r'\bwindows\b|win32|windows-sys|windows::',
            'd3d12_dxil':r'd3d12|directx.?12|dxil',
            'vulkan_spirv':r'vulkan|spir-v|spirv',
            'linux':r'\blinux\b|\bproton\b',
            'orbis':r'\borbis\b|\bps4\b',
        }.items()}
        item['files'].append({'path':path,'source_url':url,'sha256':hashlib.sha256(data).hexdigest(),
                              'markers_lexical_only':markers})
    report['projects'].append(item)
    print(repo,commit,'selected source files:',len(selected),flush=True)
output=ROOT/'research/upstream-audit.json'
output.parent.mkdir(exist_ok=True)
output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('Saved source audit; no repository code or game binaries executed.',flush=True)
