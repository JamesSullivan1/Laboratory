ptags- Process tags

This is a patch for the linux 2.6.32.60 kernel that adds support for
process tags, strings that can be associated with processes. This is
probably a generally terrible idea but it's a fun toy and enables some
interesting process control semantics (kill by tag, for example).

