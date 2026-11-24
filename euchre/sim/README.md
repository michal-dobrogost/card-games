# 28-card bid euchre simulator: parallel data generator

This generates the data the analysis runs on, using every core on your machine. Send me the CSV files it produces, and I'll rerun the auction and scoring analysis on the combined, larger data set.

## Files

- `e7.c` is the simulator: an exact (perfect-play) solver plus realistic hidden-card play.
- `run_parallel.py` builds `e7`, runs one copy per core, shows progress and merges the results.
- `brute7.py` is an optional slow check of the solver.

## Requirements

- **Python 3.7 or newer.** No extra packages are needed.
- **A C compiler:**
  - **Linux:** `gcc`
  - **Mac:** `xcode-select --install`, which gives you `clang`
  - **Windows:** use WSL, or MinGW `gcc`

The launcher builds `e7` automatically the first time you run it.

## Quick start

```
python3 run_parallel.py all
```

That runs the four recommended data sets one after another:

| Data set | What it simulates | Default deals | Time per deal on one core |
|---|---|---|---|
| `table` | Perfect-play tricks for every seat and every trump choice (4 suits, high, low) | 40,000 | ~0.3 s |
| `suit` | Random suit contracts played four ways: plain, salt, pepper, salt & pepper | 8,000 | ~1.3 s |
| `suit4` | The same, for makers with 4 or more trumps (the hands that call) | 8,000 | ~1.6 s |
| `high` | High contracts with the maker holding 2 or more aces, played four ways | 8,000 | ~1.3 s |

The full set is about 12–13 hours of single-core work. That's roughly 1.5–2 hours on 8 cores, or 3–4 hours on 4. The launcher prints progress and an estimate of the time left. It's a good job to run overnight.

Low contracts don't need their own run. Reversing the ranks turns a low hand into a high hand, so I derive low from `high`.

## Running one data set, or more data

```
python3 run_parallel.py suit4 --deals 20000 --seed 2
python3 run_parallel.py table --deals 100000 --workers 6
```

| Option | Meaning |
|---|---|
| `--deals N` | Number of deals |
| `--workers N` | Processes to run at once (default: all cores; use fewer to keep the computer responsive) |
| `--seed S` | Starting random seed (default 1). **Use a different seed for every batch you send**, or the deals will repeat. |
| `--samples K` | Hidden-card layouts each player tries per decision (default 12, matching my runs). More is slower but plays better. |
| `--out FILE` | Output file name (default `<dataset>_seed<S>.csv`) |

If you stop a run with Ctrl-C, the launcher keeps every deal finished so far and writes them to the output file.

## What to send back

Send the CSV files (`table_seed1.csv`, `suit_seed1.csv`, `suit4_seed1.csv`, `high_seed1.csv`, …). Zip them if they're large. Partial runs are useful too.

## How the play is modelled

- **Deck:** A K Q J 10 9 8 in each suit, 7 cards each, 7 tricks. Seat 3 is the maker and leads.
- **Trump:** in `suit` and `suit4`, the maker picks trump with a point count. High uses A-high no-trump.
- **Players:** each sees only his own cards. Before every play he tries 12 random layouts of the hidden cards that fit what he knows, solves each one exactly, and plays the card that does best on average.
- **Salt:** both partnerships swap one card at the same time.
  - The maker's partner passes his best trump (in no-trump, an ace). The maker passes a low side card.
  - The defenders consolidate: a defender with 2 or more trumps (in no-trump, 2 or more aces and kings) passes his worst card; one with fewer passes his best.
- **Pepper:** the maker's partner sits out.

## Output columns

**`table`:** `deal`, the four hand masks (`h0`–`h3`, bit *c* = card *c*), then tricks for `seatS_clubs … seatS_low` when that seat is maker and leads.

Card number = suit × 7 + rank:
- suits: 0 clubs, 1 spades, 2 hearts, 3 diamonds
- ranks: 0 = 8, 1 = 9, 2 = 10, 3 = J, 4 = Q, 5 = K, 6 = A

**`suit`, `suit4`, `high`:**
- `trump` (0–3 suit, 4 high)
- the maker's trumps, bowers, side aces, aces and kings, and partner's trumps
- `dd0`: perfect-play tricks
- tricks taken with realistic play under `none`, `salt`, `pepper` and `saltpepper`

## Optional: checking the solver

```
./e7 brute 1 7
python3 brute7.py <the four numbers printed> 0
```

The second command searches every line of play in Python, which takes a few minutes. It should print the same number as the first trick count that `e7 brute` printed (clubs). Change the last argument to 1–5 to check the other trump choices.
