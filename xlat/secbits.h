/* Generated by ./xlat/gen.sh from ./xlat/secbits.in; do not edit. */

static const struct xlat secbits[] = {
#if !(defined(SECBIT_NOROOT) || (defined(HAVE_DECL_SECBIT_NOROOT) && HAVE_DECL_SECBIT_NOROOT))
# define SECBIT_NOROOT (1 << 0)
#endif
 XLAT(SECBIT_NOROOT),
#if !(defined(SECBIT_NOROOT_LOCKED) || (defined(HAVE_DECL_SECBIT_NOROOT_LOCKED) && HAVE_DECL_SECBIT_NOROOT_LOCKED))
# define SECBIT_NOROOT_LOCKED (1 << 1)
#endif
 XLAT(SECBIT_NOROOT_LOCKED),
#if !(defined(SECBIT_NO_SETUID_FIXUP) || (defined(HAVE_DECL_SECBIT_NO_SETUID_FIXUP) && HAVE_DECL_SECBIT_NO_SETUID_FIXUP))
# define SECBIT_NO_SETUID_FIXUP (1 << 2)
#endif
 XLAT(SECBIT_NO_SETUID_FIXUP),
#if !(defined(SECBIT_NO_SETUID_FIXUP_LOCKED) || (defined(HAVE_DECL_SECBIT_NO_SETUID_FIXUP_LOCKED) && HAVE_DECL_SECBIT_NO_SETUID_FIXUP_LOCKED))
# define SECBIT_NO_SETUID_FIXUP_LOCKED (1 << 3)
#endif
 XLAT(SECBIT_NO_SETUID_FIXUP_LOCKED),
#if !(defined(SECBIT_KEEP_CAPS) || (defined(HAVE_DECL_SECBIT_KEEP_CAPS) && HAVE_DECL_SECBIT_KEEP_CAPS))
# define SECBIT_KEEP_CAPS (1 << 4)
#endif
 XLAT(SECBIT_KEEP_CAPS),
#if !(defined(SECBIT_KEEP_CAPS_LOCKED) || (defined(HAVE_DECL_SECBIT_KEEP_CAPS_LOCKED) && HAVE_DECL_SECBIT_KEEP_CAPS_LOCKED))
# define SECBIT_KEEP_CAPS_LOCKED (1 << 5)
#endif
 XLAT(SECBIT_KEEP_CAPS_LOCKED),
 XLAT_END
};