# H2Loop Assistant — GPL-3.0 Compliance Evidence Snapshot

This repository is a **forensic snapshot** of
https://github.com/dinesh-ramailo/h2loop-assistant
captured on 2026-04-19.

## Purpose

The upstream repository is an unacknowledged modification of
[QodeAssist](https://github.com/Palm1r/QodeAssist), which I (Petr Mironychev)
authored and maintain under GPL-3.0.

This snapshot preserves the repository state at a specific point in time
for evidentiary purposes related to GPL-3.0 compliance.

## Identified compliance issues

1. **Removal of original copyright notices.** In `QodeAssist.json.in`
   (commit history preserved in the `feat/h2loop-rebranding` branch of
   this snapshot), the original copyright was modified from:

   ```
   "Copyright": "(C) ${IDE_COPYRIGHT_YEAR} Petr Mironychev, (C) ${IDE_COPYRIGHT_YEAR} The Qt Company Ltd"
   ```

   to:

   ```
   "Copyright": "(C) ${IDE_COPYRIGHT_YEAR} H2Loop"
   ```

   This removes both my own copyright and the copyright of The Qt Company Ltd.
<img width="1369" height="530" alt="Screenshot 2026-04-19 at 15 55 37" src="https://github.com/user-attachments/assets/cb97f23d-02e6-4282-a406-2c4fcc26f9cb" />

2. **Missing modification notice** (GPL-3.0 §5a). The README contains no
   prominent notice stating that this work is a modified fork of QodeAssist,
   nor does it state the date of modification.

3. **Version reset to 0.0.1** from the then-current 0.7.1, presenting the
   work as a new independent product.

4. **UI-level rebranding** (e.g., `ChatView::ChatView()` title changed from
   "QodeAssist Chat" to "H2Loop Assistant Chat") removes end-user visibility
   of the original project origin.

## Repository analysis at time of snapshot

### Preserved original commit history

The upstream repository contains the **full original commit history** of
QodeAssist, including the earliest "Initial commit" authored by Petr
Mironychev. This establishes that the repository is a derivative work of
QodeAssist rather than an independent implementation.

### Release cadence

The snapshot contains 7 dated release branches spanning November 14,
2025 through December 22, 2025, indicating an active product release
cycle rather than experimental work:

- `release-nov-14`, `release-nov-25`, `release-dec-3`, `release-dec-10`,
  `release-dec-12`, `release-dec-17`, `release-dec-22`

The earliest release branch (`release-nov-14`) starts directly from my
original QodeAssist commits, with no independent development preceding it.

### Authors across all branches

Contributions to the modified repository come from authors associated
with the downstream H2Loop project:

- Dinesh Bala (`dinesh-ramailo` / `baladnes@gmail.com`)
- Pulkit (`code-sherpa`)
- J-ratio (`f20201505@goa.bits-pilani.ac.in`) — BITS Pilani Goa
- VedantNipane

The preserved history additionally contains contributions from upstream
QodeAssist contributors whose copyright was affected by the rebrand:

- Petr Mironychev (`Palm1r`) — original author
- Agron (`as9902613`)
- SidneyCogdill
- Mariusz Jaskółka (`jm4R`)
- Povilas Kanapickas (`radix.lt`)

### Scale of modification

Commits on `release-dec-22` (latest release branch) that do not exist on
my upstream `main`: **250 commits**.

## Integrity

- Snapshot method: `git clone --mirror` + `git push --mirror`
- Snapshot date (UTC): 2026-04-19
- Branches captured at snapshot (via `git bundle list-heads`):

  | Branch | Commit SHA |
  |---|---|
  | `edit-file-tool-test` | `09bd91968fe4707d0263e24916849e33ce4ad83c` |
  | `feat/build-for-qc16.0.0` | `deef8bf14065ac7b47c0d0c91778bd5fb8e803b5` |
  | `feat/h2loop-rebranding` | `fe539e8010a2a12d6f839cfa8a5f233deeb45446` |
  | `feat/improved-inline-ui` | `c3f3ea89887351933aa0dd3ea6f81fa9a0f97b4f` |
  | `main` | `608103b92ed005e17630a62358f0233f0c7f14d2` |
  | `release-dec-10` | `3397153d0b57d239603e890a670b1ad90377e829` |
  | `release-dec-12` | `6eb57ff3827b26b0b8f063be4095b00ffb60a89a` |
  | `release-dec-17` | `c25e69fde394599fcd37813e23505b2605173f0a` |
  | `release-dec-22` | `7c1c6f6058ef42484aea514de785dcea84394317` |
  | `release-dec-3` | `046cf2838371d6bc871f4a655057f3010efeb883` |
  | `release-nov-14` | `d80f41e948899073b72476875b143a3244c8e8fc` |
  | `release-nov-25` | `ae74ebfc705c1c73a26a9d59b0640ddd35932e37` |
  | `release-nov-25-edit-file-tool` | `6de34e94eccbe84f7c07f1056e5aa9884007c5b8` |
  | `HEAD` | `fe539e8010a2a12d6f839cfa8a5f233deeb45446` |

A git bundle of the complete repository (all refs, all history) is
preserved locally. SHA-256 of the bundle is available upon request for
verification purposes.

## Canonical upstream

The official, maintained version of this software:
**https://github.com/Palm1r/QodeAssist**

---

*Maintained by Petr Mironychev, original author of QodeAssist.*
*Contact: via GitHub [@Palm1r](https://github.com/Palm1r)*
