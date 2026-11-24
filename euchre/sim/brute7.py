"""Brute-force check of e7's perfect-play solver (slow, pure Python).
Usage:  ./e7 brute 1 SEED            -> prints 4 hand masks, then tricks for clubs..diamonds, high, low
        python3 brute7.py M0 M1 M2 M3 STRAIN   -> should print the same number for that strain (0-5)
"""
import functools, sys
H=[int(x) for x in sys.argv[1:5]]; s=int(sys.argv[5])
NR=7
es=[c//NR for c in range(28)]; pw=[(NR-1-c%NR) if s==5 else c%NR for c in range(28)]
if s<4:
    for c in range(28):
        su,r=c//NR,c%NR
        if r==3 and su==s: pw[c]=120
        elif r==3 and su==(s^1): es[c]=s; pw[c]=119
        elif su==s: pw[c]=100+r
istr=lambda c: s<4 and es[c]==s
def beats(a,b,led):
    ta,tb=istr(a),istr(b)
    if ta and tb: return pw[a]>pw[b]
    if ta: return True
    if tb: return False
    if es[a]!=led: return False
    if es[b]!=led: return True
    return pw[a]>pw[b]
hs=tuple(frozenset(c for c in range(28) if h>>c&1) for h in H)
@functools.lru_cache(None)
def solve(h,leader):
    if not h[0]: return 0
    def rec(h,pos,led,win,wc,alpha,beta):
        if pos==4: return (1 if win in (1,3) else 0)+solve(h,win)
        p=(leader+pos)%4
        legal=[c for c in h[p] if pos>0 and es[c]==led] or list(h[p])
        mx=p in (1,3); best=-1 if mx else 99
        for c in legal:
            nh=list(h); nh[p]=h[p]-{c}; nh=tuple(nh)
            if pos==0: v=rec(nh,1,es[c],p,c,alpha,beta)
            elif beats(c,wc,led): v=rec(nh,pos+1,led,p,c,alpha,beta)
            else: v=rec(nh,pos+1,led,win,wc,alpha,beta)
            if mx: best=max(best,v); alpha=max(alpha,best)
            else: best=min(best,v); beta=min(beta,best)
            if alpha>=beta: break
        return best
    return rec(h,0,None,None,None,-1,99)
print(s,solve(hs,3))
