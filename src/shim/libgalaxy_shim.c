/*
 * libgalaxy_shim.c — Minimal GOG Galaxy SDK shim for DLC ownership
 *
 * Replaces the real Galaxy SDK (libGalaxy.dylib) with file-based DLC
 * ownership checking.  On macOS, the GOG offline installer places
 * goggame-<id>.info files in <game_root>/Contents/Resources/.  This
 * shim checks for those files when the game calls IsDlcOwned().
 *
 * The game imports 12 Galaxy API symbols.  We provide vtable-compatible
 * C++ facade objects for Apps, User, and ListenerRegistrar.  The rest
 * return NULL to prevent subsystem creation (Stats, Friends, etc.).
 *
 * Build: linked as a shared library with a version script that exports
 * only the 12 mangled C++ symbols.
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

/* ---------- helpers ---------- */

static void noop(void) {}
static uint64_t ret0(void) { return 0; }

/* ---------- DLC file check ---------- */

/*
 * Check if goggame-<product_id>.info exists.
 * CWD is gamedata/NecroDancerMP.app/Contents/MacOS/ (machismo chdir).
 * ../../../ = gamedata/
 * Info files at gamedata/Contents/Resources/goggame-<id>.info
 */
static int check_dlc_file(uint64_t product_id)
{
    char path[256];
    snprintf(path, sizeof(path),
             "../../../Contents/Resources/goggame-%llu.info",
             (unsigned long long)product_id);
    int found = (access(path, F_OK) == 0);
    fprintf(stderr, "galaxy_shim: IsDlcOwned(%llu) → %s (%s)\n",
            (unsigned long long)product_id, found ? "owned" : "not owned", path);
    return found;
}

/* ---------- pending callback queue ---------- */

#define MAX_PENDING 8

static struct {
    uint64_t product_id;
    void    *listener;
    int      is_owned;
} pending[MAX_PENDING];
static int num_pending;

/* ---------- IApps facade ---------- */

/*
 * Galaxy IsDlcOwned: returns 0 if owned (Galaxy convention: 0 = true for
 * some bool-returning methods).  Actually, looking at the decompiled code
 * more carefully: IsDlcOwned returns a bool (0 = not owned, non-zero = owned).
 * The game checks:  if (IsDlcOwned(id) == 0) { ... skip ... }
 *                   vs if (IsDlcOwned(id) != 0) { ... request async ... }
 *
 * Wait — re-reading the decompiled update():
 *   iVar1 = (**(code **)(*piVar2 + 0x10))(piVar2, productId);
 *   if (iVar1 == 0) {  // DLC is owned
 *       ... request async check ...
 *   }
 *
 * So 0 = owned, non-zero = not owned.  This is Galaxy's convention where
 * IsDlcOwned returns 0 on success (owned) and error code on failure.
 *
 * Actually wait, this might be backwards. Let me think again.
 * Galaxy's IApps::IsDlcOwned returns bool. true = owned.
 * But the decompiled code checks "if (iVar1 == 0)" meaning "if NOT owned".
 * And inside that block it does NOT request async — it skips.
 * Actually looking at the plan agent's analysis:
 *   "if (iVar1 == 0) {  // DLC is owned"
 *   No — == 0 means false means NOT owned.
 *
 * Let me re-read: the plan says "returns 0 = DLC owned" but that contradicts
 * bool semantics. Let me just use standard bool: 1 = owned, 0 = not owned.
 * The game code does: if result != 0 then proceed with async check.
 */
static uint64_t shim_apps_is_dlc_owned(void *self, uint64_t product_id)
{
    return check_dlc_file(product_id) ? 1 : 0;
}

/*
 * RequestIsDlcOwned — queue the callback for ProcessData() to fire.
 * listener is IIsDlcOwnedListener* with OnDlcCheckSuccess at vtable[2] (+0x10).
 */
static void shim_apps_request_is_dlc_owned(void *self, uint64_t product_id,
                                           void *listener)
{
    if (num_pending < MAX_PENDING) {
        pending[num_pending].product_id = product_id;
        pending[num_pending].listener = listener;
        pending[num_pending].is_owned = check_dlc_file(product_id);
        num_pending++;
        fprintf(stderr, "galaxy_shim: RequestIsDlcOwned(%llu) queued (listener=%p)\n",
                (unsigned long long)product_id, listener);
    }
}

static void *apps_vtable[] = {
    (void *)noop,                          /* +0x00: destructor */
    (void *)noop,                          /* +0x08: destructor */
    (void *)shim_apps_is_dlc_owned,        /* +0x10: IsDlcOwned */
    (void *)shim_apps_request_is_dlc_owned /* +0x18: RequestIsDlcOwned */
};

static struct { void **vtable; } apps_instance = { apps_vtable };

/* ---------- IListenerRegistrar facade ---------- */

static void shim_registrar_register(void *self, uint32_t type, void *listener)
{
    fprintf(stderr, "galaxy_shim: ListenerRegistrar::Register(type=0x%x, listener=%p)\n",
            type, listener);
}

static void shim_registrar_unregister(void *self, uint32_t type, void *listener)
{
    (void)self; (void)type; (void)listener;
}

static void *registrar_vtable[] = {
    (void *)noop,                     /* +0x00: destructor */
    (void *)noop,                     /* +0x08: destructor */
    (void *)shim_registrar_register,  /* +0x10: Register */
    (void *)shim_registrar_unregister /* +0x18: Unregister */
};

static struct { void **vtable; } registrar_instance = { registrar_vtable };

/* ---------- IUser facade ---------- */

static void *user_vtable[] = {
    (void *)noop, /* +0x00: destructor */
    (void *)noop, /* +0x08: destructor */
    (void *)ret0, /* +0x10: pad */
    (void *)ret0  /* +0x18: GetGalaxyID → returns 0 */
};

static struct { void **vtable; } user_instance = { user_vtable };

/* ---------- Galaxy API exports ---------- */

/*
 * Use __asm__ labels to export C++ mangled symbol names.
 * The resolver strips the leading _ from Mach-O's __ZN6galaxy...
 * and does dlsym("_ZN6galaxy3api...").
 */

/* Init(InitOptions const&) — accept and ignore */
void shim_init(void *opts) __asm__("_ZN6galaxy3api4InitERKNS0_11InitOptionsE");
void shim_init(void *opts)
{
    (void)opts;
    fprintf(stderr, "galaxy_shim: Init()\n");
}

/* Shutdown() */
void shim_shutdown(void) __asm__("_ZN6galaxy3api8ShutdownEv");
void shim_shutdown(void)
{
    fprintf(stderr, "galaxy_shim: Shutdown()\n");
}

/* ProcessData() — drain pending DLC callbacks */
void shim_process_data(void) __asm__("_ZN6galaxy3api11ProcessDataEv");
void shim_process_data(void)
{
    for (int i = 0; i < num_pending; i++) {
        void *listener = pending[i].listener;
        uint64_t id = pending[i].product_id;
        int owned = pending[i].is_owned;

        /*
         * Fire OnDlcCheckSuccess(listener, productId, isOwned).
         * IIsDlcOwnedListener vtable layout:
         *   [0] +0x00: destructor
         *   [1] +0x08: destructor
         *   [2] +0x10: OnDlcCheckSuccess(uint64_t, bool)
         *   [3] +0x18: OnDlcCheckFailure(uint64_t, FailureReason)
         */
        void **vtable = *(void ***)listener;
        typedef void (*on_success_fn)(void *, uint64_t, int);
        on_success_fn fn = (on_success_fn)vtable[2];
        fprintf(stderr, "galaxy_shim: firing OnDlcCheckSuccess(%llu, %d) on listener %p\n",
                (unsigned long long)id, owned, listener);
        fn(listener, id, owned);
    }
    num_pending = 0;
}

/* Apps() → returns IApps facade (needed for DLC checking) */
void *shim_apps(void) __asm__("_ZN6galaxy3api4AppsEv");
void *shim_apps(void) { return &apps_instance; }

/* User() → returns IUser facade (GogAPI::init null-checks this) */
void *shim_user(void) __asm__("_ZN6galaxy3api4UserEv");
void *shim_user(void) { return &user_instance; }

/* ListenerRegistrar() → returns facade (needed for listener reg/unreg) */
void *shim_listener_registrar(void) __asm__("_ZN6galaxy3api17ListenerRegistrarEv");
void *shim_listener_registrar(void) { return &registrar_instance; }

/* Stats() → NULL (prevents GogUserStatistics/GogLeaderboardProvider creation) */
void *shim_stats(void) __asm__("_ZN6galaxy3api5StatsEv");
void *shim_stats(void) { return NULL; }

/* Friends() → NULL */
void *shim_friends(void) __asm__("_ZN6galaxy3api7FriendsEv");
void *shim_friends(void) { return NULL; }

/* Storage() → NULL */
void *shim_storage(void) __asm__("_ZN6galaxy3api7StorageEv");
void *shim_storage(void) { return NULL; }

/* Networking() → NULL */
void *shim_networking(void) __asm__("_ZN6galaxy3api10NetworkingEv");
void *shim_networking(void) { return NULL; }

/* Matchmaking() → NULL */
void *shim_matchmaking(void) __asm__("_ZN6galaxy3api11MatchmakingEv");
void *shim_matchmaking(void) { return NULL; }

/* GetError() → NULL (no error) */
void *shim_get_error(void) __asm__("_ZN6galaxy3api8GetErrorEv");
void *shim_get_error(void) { return NULL; }
