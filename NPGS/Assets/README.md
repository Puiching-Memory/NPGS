# Asset provenance policy

Assets are release inputs, not disposable build output. Every newly added font,
model, texture, audio file, or data table must include enough information to
redistribute it:

- original source URL or publication;
- author or data producer;
- license and required attribution;
- whether the file was modified;
- a stable version, release date, or retrieval date.

Keep that information in a `LICENSE`, `source_attribution.txt`, or `source.txt`
file beside the asset. Generated files belong under `Cache` or the build tree and
must not be committed.

## Audit status

The existing model attribution files are retained beside their models. The
redistribution status of `Fonts/msyh*.ttc` and `Fonts/seguisym.ttf` has not yet
been verified; do not publish a release containing those fonts until their
license has been reviewed or they have been replaced with redistributable fonts.
