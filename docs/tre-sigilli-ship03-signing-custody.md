# Tre Sigilli — SHIP-03 · Android Signing Custody

Languages: [English](tre-sigilli-ship03-signing-custody.md) | [Italiano](tre-sigilli-ship03-signing-custody.it.md)

Status: DRAFT — no signing key generated.

## Signing identity

The Android release key identifies distributable Tre Sigilli versions.
It must remain stable for updates signed with the same identity.

Losing the key may prevent updates to installations already distributed.

## Canonical locations

Signing material:
`~/.local/share/giadaware/tre-sigilli/signing/`

Private configuration:
`~/.config/giadaware/tre-sigilli/`

Both locations are outside the BoardLab repository.

Never store keys or passwords in repositories, public documentation,
CI workflows, shell history or logs.

## Permissions and isolation

- Private directories: 0700.
- Keystores and private configuration files: 0600.
- No symbolic links to repositories or public locations.
- No copies of the keystore under `android/`.
- No plaintext passwords passed as command arguments.
- `.gitignore` is an additional safeguard, not the primary defense.

## Backup and recovery

Before distributing the first signed APK:

1. Prepare at least one encrypted recovery copy,
   stored separately from the primary system.
2. Store credentials needed to access that copy separately.
3. Verify that the backup can be read without replacing
   or modifying the original key.
4. Record the public certificate fingerprint without
   exposing private material.
5. Document the restoration procedure and verify
   the availability of the recovery medium.

The backup medium must be agreed upon before
generating the definitive signing key.

## Signing and verification

- Use official Android SDK tools.
- Verify the APK with `apksigner verify`.
- Record the APK SHA-256 and certificate fingerprint.
- Verify installation and updates on the qualified AVD.
- Keep distributable APKs, private keys and passwords separate.

## Authorizations and gates

Git protection and this policy do not authorize key generation.

`SIGNING_CUSTODY_POLICY=PASS` requires document review.

`KEY_GENERATION_GATE` requires explicit authorization.

`M2_SIGNING=PASS` requires verified signing, protected custody
and a qualified recovery plan.

No APK is published or distributed automatically.
