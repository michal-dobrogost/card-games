#!/usr/bin/env python3
"""Run the 28-card bid euchre simulator (e7) on every CPU core and merge the results.

Examples
  python3 run_parallel.py table  --deals 40000
  python3 run_parallel.py suit   --deals 8000
  python3 run_parallel.py suit4  --deals 8000
  python3 run_parallel.py high   --deals 8000
  python3 run_parallel.py all                     # the recommended full set, one after another

Options
  --workers N   processes to run at once (default: number of CPU cores)
  --seed S      base random seed (default 1); use a different seed for each batch you send
  --samples K   hidden-card layouts per decision in play (default 12; higher = better play, slower)
  --out FILE    output CSV (default: <dataset>_seed<S>.csv)

Standard library only. Needs a C compiler (gcc, clang or cc) the first time, to build e7.
"""
import argparse, os, sys, subprocess, time, shutil, platform

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, 'e7.exe' if platform.system() == 'Windows' else 'e7')
PRESETS = {
    # name: (arguments after "DEALS SEED", description, recommended deals)
    'table': (['table'], 'perfect-play tricks for every seat and trump choice', 40000),
    'suit':  (['sp', '-1', '0'], 'random suit contracts played 4 ways (plain/salt/pepper/both)', 8000),
    'suit4': (['sp', '-1', '4'], 'suit contracts where the maker has 4+ trumps, played 4 ways', 8000),
    'high':  (['sp', '4', '2'], 'high (no-trump) contracts where the maker has 2+ aces, played 4 ways', 8000),
}

def build():
    if os.path.exists(EXE):
        return
    src = os.path.join(HERE, 'e7.c')
    for cc in ('gcc', 'clang', 'cc'):
        if shutil.which(cc):
            print(f'building e7 with {cc} ...', flush=True)
            subprocess.check_call([cc, '-O2', '-o', EXE, src])
            return
    sys.exit('No C compiler found. Install gcc or clang (Windows: MinGW or WSL), then run again.')

def count_lines(path):
    try:
        with open(path, 'rb') as f:
            return max(sum(1 for _ in f) - 1, 0)          # minus the header
    except FileNotFoundError:
        return 0

def run(dataset, deals, workers, seed, samples, out):
    mode_args, desc, _ = PRESETS[dataset]
    out = out or os.path.join(HERE, f'{dataset}_seed{seed}.csv')
    workers = max(1, min(workers, deals))
    chunks = [deals // workers + (1 if i < deals % workers else 0) for i in range(workers)]
    parts = [f'{out}.part{i}' for i in range(workers)]
    procs = []
    print(f'\n{dataset}: {desc}\n  {deals} deals on {workers} processes -> {out}', flush=True)
    t0 = time.time()
    try:
        for i, (n, part) in enumerate(zip(chunks, parts)):
            wseed = seed * 1000 + i + (0 if dataset == 'table' else 500)
            args = [EXE, mode_args[0], str(n), str(wseed)] + mode_args[1:]
            if mode_args[0] == 'sp':
                args.append(str(samples))
            procs.append(subprocess.Popen(args, stdout=open(part, 'w')))
        while any(p.poll() is None for p in procs):
            time.sleep(10)
            done = sum(count_lines(p) for p in parts)
            el = time.time() - t0
            eta = el / done * (deals - done) if done else float('nan')
            print(f'  {done}/{deals} deals ({100 * done / deals:.0f}%), elapsed {el / 60:.1f} min, '
                  f'about {eta / 60:.1f} min left', flush=True)
        bad = [p.returncode for p in procs if p.returncode != 0]
        if bad:
            print(f'  warning: {len(bad)} process(es) exited with an error', flush=True)
    except KeyboardInterrupt:
        print('\n  stopped; merging the deals finished so far', flush=True)
        for p in procs:
            p.terminate()
    # merge: one header, renumber deals, drop any half-written last line
    n_out = 0
    with open(out, 'w') as fo:
        header_done = False
        for part in parts:
            if not os.path.exists(part):
                continue
            with open(part) as fi:
                lines = fi.read().split('\n')
            if not lines or not lines[0]:
                continue
            if not header_done:
                fo.write(lines[0] + '\n'); header_done = True
            ncol = lines[0].count(',')
            for ln in lines[1:]:
                if ln and ln.count(',') == ncol:
                    fo.write(f'{n_out},' + ln.split(',', 1)[1] + '\n'); n_out += 1
            os.remove(part)
    print(f'  wrote {n_out} deals to {out} in {(time.time() - t0) / 60:.1f} min', flush=True)

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('dataset', choices=list(PRESETS) + ['all'])
    ap.add_argument('--deals', type=int)
    ap.add_argument('--workers', type=int, default=os.cpu_count() or 1)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--samples', type=int, default=12)
    ap.add_argument('--out')
    a = ap.parse_args()
    build()
    names = list(PRESETS) if a.dataset == 'all' else [a.dataset]
    for name in names:
        deals = a.deals or PRESETS[name][2]
        run(name, deals, a.workers, a.seed, a.samples, None if a.dataset == 'all' else a.out)

if __name__ == '__main__':
    main()
