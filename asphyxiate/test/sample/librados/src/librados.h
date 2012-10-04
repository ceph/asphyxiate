#ifndef CEPH_LIBRADOS_H
#define CEPH_LIBRADOS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <netinet/in.h>
#if defined(__linux__)
#include <linux/types.h>
#elif defined(__FreeBSD__)
#include <sys/types.h>
#include "include/inttypes.h"
#endif
#include <string.h>

#ifndef CEPH_OSD_TMAP_SET
/* These are also defined in rados.h and objclass.h. Keep them in sync! */
#define CEPH_OSD_TMAP_HDR 'h'
#define CEPH_OSD_TMAP_SET 's'
#define CEPH_OSD_TMAP_CREATE 'c'
#define CEPH_OSD_TMAP_RM  'r'
#endif

#define LIBRADOS_VER_MAJOR 0
#define LIBRADOS_VER_MINOR 30
#define LIBRADOS_VER_EXTRA 0

#define LIBRADOS_VERSION(maj, min, extra) ((maj << 16) + (min << 8) + extra)

#define LIBRADOS_VERSION_CODE LIBRADOS_VERSION(LIBRADOS_VER_MAJOR, LIBRADOS_VER_MINOR, LIBRADOS_VER_EXTRA)

#define LIBRADOS_SUPPORTS_WATCH 1

/**
 * @defgroup xattr_comp xattr comparison operations
 * BUG: there's no way to use these in the C api
 * @{
 */
/* enum { */
/* 	LIBRADOS_CMPXATTR_OP_NOP = 0, */
/* 	LIBRADOS_CMPXATTR_OP_EQ  = 1, */
/* 	LIBRADOS_CMPXATTR_OP_NE  = 2, */
/* 	LIBRADOS_CMPXATTR_OP_GT  = 3, */
/* 	LIBRADOS_CMPXATTR_OP_GTE = 4, */
/* 	LIBRADOS_CMPXATTR_OP_LT  = 5, */
/* 	LIBRADOS_CMPXATTR_OP_LTE = 6 */
/* }; */
/** @} */

struct CephContext;

/**
 * A handle for interacting with a RADOS cluster. It encapsulates all
 * RADOS client configuration, including username, key for
 * authentication, logging, and debugging. Talking different clusters
 * -- or to the same cluster with different users -- requires
 * different cluster handles.
 */
typedef void *rados_t;

/**
 * An io context encapsulates a few settings for all I/O operations
 * done on it: TODO
 * - pool - set when the io context is created (see rados_ioctx_open())
 * - snapshot context for writes (see
 *   rados_ioctx_selfmanaged_snap_set_write_context())
 * - snapshot id to read from (see rados_ioctx_set_snap_read())
 * - object locater for all single-object operations (see
 *   rados_ioctx_set_object_locater_key())
 *
 * @warning changing any of these settings is not thread-safe -
 * librados users must synchronize any of these changes on their own,
 * or use separate io contexts for each thread
 */
typedef void *rados_ioctx_t;

/**
 * An iterator for listing the objects in a pool.
 * Used with rados_ioctx_objects_list_open(),
 * rados_ioctx_objects_list_next(), and
 * rados_ioctx_objects_list_close().
 */
typedef void *rados_list_ctx_t;

/**
 * The id of a snapshot.
 */
typedef uint64_t rados_snap_t;

/**
 * An iterator for listing extended attrbutes on an object.
 * Used with rados_getxattrs(), rados_getxattrs_next(), and
 * rados_getxattrs_end().
 */
typedef void *rados_xattrs_iter_t;

/**
 * Usage information for a pool.
 */
struct rados_pool_stat_t {
  uint64_t num_bytes;    // in bytes
  uint64_t num_kb;       // in KB
  uint64_t num_objects;
  uint64_t num_object_clones;
  uint64_t num_object_copies;  // num_objects * num_replicas
  uint64_t num_objects_missing_on_primary;
  uint64_t num_objects_unfound;
  uint64_t num_objects_degraded;
  uint64_t num_rd, num_rd_kb,num_wr, num_wr_kb;
};

/**
 * @struct rados_cluster_stat_t
 * Cluster-wide usage information
 */
struct rados_cluster_stat_t {
  uint64_t kb, kb_used, kb_avail;
  uint64_t num_objects;
};

/**
 * Get the version of librados.
 *
 * The version number is major.minor.extra. Note that this is
 * unrelated to the Ceph version number.
 *
 * TODO: define version semantics, i.e.:
 *   incrementing major is for backwards-incompatible changes
 *   incrementing minor is for backwards-compatible changes
 *   incrementing extra is for bug fixes
 *
 * @param major where to store the major version number
 * @param minor where to store the minor version number
 * @param extra where to store the extra version number
 */
void rados_version(int *major, int *minor, int *extra);

/**
 * @defgroup init Setup and Teardown
 * These are the first and last functions to that should be called
 * when using librados.
 *
 * @{
 */

/**
 * Create a handle for communicating with a RADOS cluster.
 *
 * Ceph environment variables are read when this is called, so if
 * $CEPH_ARGS specifies everything you need to connect, no further
 * configuration is necessary.
 *
 * @param cluster where to store the handle
 * @param id the user to connect as (i.e. admin, *not* client.admin)
 * @return 0 on success, negative error code on failure
 */
int rados_create(rados_t *cluster, const char * const id);

/**
 * Initialize a cluster handle from an existing configuration.
 *
 * Copies all configuration, as retrieved by the C++ API.
 *
 * BUG: Since CephContext isn't accessible from the C API, this function is useless
 *
 * @param cluster where to store the handle
 * @param cct_ the existing configuration to use
 * @return 0 on success, negative error code on failure
 */
int rados_create_with_context(rados_t *cluster, struct CephContext *cct_);

/**
 * Connect to the cluster.
 *
 * BUG: Before calling this, calling a function that communicates with the
 * cluster will crash.
 *
 * @pre The cluster handle is configured with at least a monitor
 * address. If cephx is enabled, a client name and secret must also be
 * set.
 *
 * @post If this succeeds, any function in librados may be used
 *
 * @param cluster The cluster to connect to.
 * @return 0 on sucess, negative error code on failure
 */
int rados_connect(rados_t cluster);

/**
 * Disconnects from the cluster.
 *
 * For clean up, this is only necessary after rados_connect() has
 * succeeded.
 *
 * @warning This does not guarantee any asynchronous writes have
 * completed. To do that, you must call rados_aio_flush() on all open
 * io contexts.
 *
 * @post the cluster handle cannot be used again
 *
 * @param cluster the cluster to shutdown
 */
void rados_shutdown(rados_t cluster);

/** @} init */

/**
 * @defgroup config Configuration
 * These functions read and update Ceph configuration for a cluster
 * handle. Any configuration changes must be done before connecting to
 * the cluster.
 *
 * Options that librados users might want to set include:
 * - mon_host
 * - auth_supported
 * - key, keyfile, or keyring when using cephx
 * - log_file, log_to_stderr, err_to_stderr, and log_to_syslog
 * - debug_rados, debug_objecter, debug_monc, debug_auth, or debug_ms
 *
 * All possible options can be found in src/common/config_opts.h in ceph.git
 *
 * @{
 */

/**
 * Configure the cluster handle using a Ceph config file
 *
 * If path is NULL, the default locations are searched, and the first
 * found is used. The locations are:
 * - $CEPH_CONF (environment variable)
 * - /etc/ceph/ceph.conf
 * - ~/.ceph/config
 * - ceph.conf (in the current working directory)
 *
 * @pre rados_connect() has not been called on the cluster handle
 *
 * @param cluster cluster handle to configure
 * @param path path to a Ceph configuration file
 * @return 0 on success, negative error code on failure
 */
int rados_conf_read_file(rados_t cluster, const char *path);

/**
 * Configure the cluster handle with command line arguments
 *
 * argv can contain any common Ceph command line option, including any
 * configuration parameter prefixed by '--' and replacing spaces with
 * dashes or underscores. For example, the following options are equivalent:
 * - --mon-host 10.0.0.1:6789
 * - --mon_host 10.0.0.1:6789
 * - -m 10.0.0.1:6789
 *
 * @pre rados_connect() has not been called on the cluster handle
 *
 * @param cluster cluster handle to configure
 * @param argc number of arguments in argv
 * @param argv arguments to parse
 * @return 0 on success, negative error code on failure
 */
int rados_conf_parse_argv(rados_t cluster, int argc, const char **argv);

/**
 * Configure the cluster handle based on an environment variable
 *
 * The contents of the environment variable are parsed as if they were
 * Ceph command line options. If var is NULL, the CEPH_ARGS
 * environment variable is used.
 *
 * @pre rados_connect() has not been called on the cluster handle
 *
 * BUG: this is not threadsafe - it uses a static buffer
 *
 * @param cluster cluster handle to configure
 * @param var name of the environment variable to read
 * @return 0 on success, negative error code on failure
 */
int rados_conf_parse_env(rados_t cluster, const char *var);

/**
 * Set a configuration option
 *
 * @pre rados_connect() has not been called on the cluster handle
 *
 * @param cluster cluster handle to configure
 * @param option option to set
 * @param value value of the option
 * @return 0 on success, negative error code on failure. -ENOENT is
 * returned when the option is not a Ceph configuration option.
 */
int rados_conf_set(rados_t cluster, const char *option, const char *value);

/**
 * Get the value of a configuration option
 *
 * @param cluster configuration to read
 * @param option which option to read
 * @param buf where to write the configuration value
 * @param len the size of buf in bytes
 * @return 0 on success, negative error code on failure.
 * -ENAMETOOLONG is returned if the buffer is too short to contain the
 * requested value.
 */
int rados_conf_get(rados_t cluster, const char *option, char *buf, size_t len);

/** @} config */

/**
 * Read usage info about the cluster
 *
 * This tells you total space, space used, space available, and number
 * of objects. These are not updated immediately when data is written,
 * they are eventually consistent.
 *
 * @param cluster cluster to query
 * @param result where to store the results
 * @return 0 on success, negative error code on failure
 */
int rados_cluster_stat(rados_t cluster, struct rados_cluster_stat_t *result);


/**
 * @defgroup pools Pools
 *
 * RADOS pools are separate namespaces for objects. Pools may have
 * different crush rules associated with them, so they could have
 * differing replication levels or placement strategies. RADOS
 * permissions are also tied to pools - users can have different read,
 * write, and execute permissions on a per-pool basis.
 *
 * @{
 */

/**
 * List objects in a pool
 *
 * Gets a list of pool names as NULL-terminated strings.  The pool
 * names will be placed in the supplied buffer one after another.
 * After the last pool name, there will be two 0 bytes in a row.
 *
 * If len is too short to fit all the pool name entries we need, we will fill
 * as much as we can.
 *
 * @param cluster cluster handle
 * @param buf output buffer
 * @param len output buffer length
 * @return length of the buffer we would need to list all pools
 */
int rados_pool_list(rados_t cluster, char *buf, size_t len);

/**
 * Create an io context
 *
 * The io context allows you to perform operations within a particular
 * pool. For more details see rados_ioctx_t.
 *
 * @param cluster which cluster the pool is in
 * @param pool_name name of the pool
 * @param ioctx where to store the io context
 * @return 0 on success, negative error code on failure
 */
int rados_ioctx_create(rados_t cluster, const char *pool_name, rados_ioctx_t *ioctx);

/**
 * The opposite of rados_ioctx_create
 *
 * This just tells librados that you no longer need to use the io context.
 * It may not be freed immediately if there are pending asynchronous
 * requests on it, but you should not use an io context again after
 * calling this function on it.
 *
 * @warning This does not guarantee any asynchronous
 * writes have completed. You must call rados_aio_flush()
 * on the io context before destroying it to do that.
 *
 * @param io the io context to dispose of
 */
void rados_ioctx_destroy(rados_ioctx_t io);

/**
 * Get pool usage statistics
 *
 * Fills in a rados_pool_stat_t after querying the cluster.
 *
 * @param io determines which pool to query
 * @param stats where to store the results
 * @return 0 on success, negative error code on failure
 */
int rados_ioctx_pool_stat(rados_ioctx_t io, struct rados_pool_stat_t *stats);

/**
 * Get the id of a pool
 *
 * @param cluster which cluster the pool is in
 * @param pool_name which pool to look up
 * @return id of the pool
 * @return -ENOENT if the pool is not found
 */
int64_t rados_pool_lookup(rados_t cluster, const char *pool_name);

/**
 * Create a pool with default settings
 *
 * @param cluster the cluster in which the pool will be created
 * @param pool_name the name of the new pool
 * @return 0 on success, negative error code on failure
 */
int rados_pool_create(rados_t cluster, const char *pool_name);

/**
 * Create a pool owned by a specific auid
 *
 * The auid is the authenticated user id to give ownership of the pool.
 * TODO: document auid and the rest of the auth system
 *
 * @param cluster the cluster in which the pool will be created
 * @param pool_name the name of the new pool
 * @param auid the id of the owner of the new pool
 * @return 0 on success, negative error code on failure
 */
int rados_pool_create_with_auid(rados_t cluster, const char *pool_name, uint64_t auid);

/**
 * Create a pool with a specific CRUSH rule
 *
 * @param cluster the cluster in which the pool will be created
 * @param pool_name the name of the new pool
 * @param crush_rule which rule to use for placement in the new pool
 * @return 0 on success, negative error code on failure
 */
int rados_pool_create_with_crush_rule(rados_t cluster, const char *pool_name,
				      __u8 crush_rule);

/**
 * Create a pool with a specific CRUSH rule and auid
 *
 * This is a combination of rados_pool_create_with_crush_rule() and
 * rados_pool_create_with_auid().
 *
 * @param cluster the cluster in which the pool will be created
 * @param pool_name the name of the new pool
 * @param crush_rule which rule to use for placement in the new pool
 * @param auid the id of the owner of the new pool
 * @return 0 on success, negative error code on failure
 */
int rados_pool_create_with_all(rados_t cluster, const char *pool_name, uint64_t auid,
			       __u8 crush_rule);

/**
 * Delete a pool and all data inside it
 *
 * The pool is removed from the cluster immediately,
 * but the actual data is deleted in the background.
 *
 * @param cluster the cluster the pool is in
 * @param pool_name which pool to delete
 * @return 0 on success, negative error code on failure
 */
int rados_pool_delete(rados_t cluster, const char *pool_name);

/**
 * Attempt to change an io context's associated auid "owner."
 *
 * Requires that you have write permission on both the current and new
 * auid.
 *
 * @param io reference to the pool to change.
 * @param auid the auid you wish the io to have.
 * @return 0 on success, negative error code on failure
 */
int rados_ioctx_pool_set_auid(rados_ioctx_t io, uint64_t auid);

/**
 * Get the auid of a pool
 *
 * @param io pool to query
 * @param auid where to store the auid
 * @return 0 on success, negative error code on failure
 */
int rados_ioctx_pool_get_auid(rados_ioctx_t io, uint64_t *auid);

/** @} pools */

/**
 * Set the key for mapping objects to pgs within an io context.
 *
 * The key is used instead of the object name to determine which
 * placement groups an object is put in. This affects all subsequent
 * operations of the io context - until a different locater key is
 * set, all objects in this io context will be placed in the same pg.
 *
 * This is useful if you need to do clone_range operations, which must
 * be done with the source and destination objects in the same pg.
 *
 * @param io the io context to change
 * @param key the key to use as the object locator, or NULL to discard
 * any previously set key
 */
void rados_ioctx_locator_set_key(rados_ioctx_t io, const char *key);

/**
 * Get the pool id of the io context
 * @param io the io context to query
 * @return the id of the pool the io context uses
 */
int rados_ioctx_get_id(rados_ioctx_t io);

/**
 * @defgroup list_obj Listing Objects
 * @{
 */
/**
 * Start listing objects in a pool
 *
 * @param io the pool to list from
 * @param ctx the handle to store list context in
 * @return 0 on success, negative error code on failure
 */
int rados_objects_list_open(rados_ioctx_t io, rados_list_ctx_t *ctx);

/**
 * Get the next object name and locator in the pool
 *
 * @param ctx iterator marking where you are in the listing
 * @param entry where to store the name of the entry (caller must free)
 * @param key where to store the object locator (set to NULL to ignore) (caller must free)
 * @return 0 on success, negative error code on failure
 * @return -ENOENT when there are no more objects to list
 */
int rados_objects_list_next(rados_list_ctx_t ctx, const char **entry, const char **key);

/**
 * Close the object listing handle.
 *
 * This should be called when the handle is no longer needed.
 * The handle should not be used after it has been closed.
 *
 * @param ctx the handle to close
 */
void rados_objects_list_close(rados_list_ctx_t ctx);

/** @} Listing Objects */

/**
 * @defgroup snaps Snapshots
 *
 * RADOS snapshots are based upon sequence numbers that form a
 * snapshot context. They are pool-specific. The snapshot context
 * consists of the current snapshot sequence number for a pool, and an
 * array of sequence numbers at which snapshots were taken, in
 * descending order. Whenever a snapshot is created or deleted, the
 * snapshot sequence number for the pool is increased. To add a new
 * snapshot, the new snapshot sequence number must be increased and
 * added to the snapshot context.
 *
 * There are two ways to manage these snapshot contexts:
 * -# within the RADOS cluster
 *    These are called pool snapshots, and store the snapshot context
 *    in the OSDMap. These represent a snapshot of all the objects in
 *    a pool.
 * -# within the RADOS clients
 *    These are called self-managed snapshots, and push the
 *    responsibility for keeping track of the snapshot context to the
 *    clients. For every write, the client must send the snapshot
 *    context. In librados, this is accomplished with
 *    rados_selfmanaged_snap_set_write_context(). These are more
 *    difficult to manage, but are restricted to specific objects
 *    instead of applying to an entire pool.
 *
 * @{
 */

/**
 * Create a pool-wide snapshot
 *
 * @param io the pool to snapshot
 * @param snapname the name of the snapshot
 * @return 0 on success, negative error code on failure
 */
int rados_ioctx_snap_create(rados_ioctx_t io, const char *snapname);

/**
 * Delete a pool snapshot
 *
 * @param io the pool to delete the snapshot from
 * @param snapname which snapshot to delete
 * @return 0 on success, negative error code on failure
 */
int rados_ioctx_snap_remove(rados_ioctx_t io, const char *snapname);

/**
 * Rollback an object to a pool snapshot
 *
 * The contents of the object will be the same as
 * when the snapshot was taken.
 *
 * @param io the pool in which the object is stored
 * @param oid the name of the object to rollback
 * @param snapname which snapshot to rollback to
 * @return 0 on success, negative error code on failure
 */
int rados_rollback(rados_ioctx_t io, const char *oid,
		   const char *snapname);

/**
 * Set the snapshot from which reads are performed.
 *
 * Subsequent reads will return data as it was at the time of that
 * snapshot.
 *
 * @param io the io context to change
 * @param snap the id of the snapshot to set, or CEPH_NOSNAP for no
 * snapshot (i.e. normal operation)
 */
void rados_ioctx_snap_set_read(rados_ioctx_t io, rados_snap_t snap);

/**
 * Allocate an ID for a self-managed snapshot
 *
 * Get a unique ID to put in the snaphot context to create a
 * snapshot. A clone of an object is not created until a write with
 * the new snapshot context is completed.
 *
 * @param io the pool in which the snapshot will exist
 * @param snapid where to store the newly allocated snapshot ID
 * @return 0 on success, negative error code on failure
 */
int rados_ioctx_selfmanaged_snap_create(rados_ioctx_t io, rados_snap_t *snapid);

/**
 * Remove a self-managed snapshot
 *
 * This increases the snapshot sequence number, which will cause
 * snapshots to be removed lazily.
 *
 * @param io the pool in which the snapshot will exist
 * @param snapid where to store the newly allocated snapshot ID
 * @return 0 on success, negative error code on failure
 */
int rados_ioctx_selfmanaged_snap_remove(rados_ioctx_t io, rados_snap_t snapid);

/**
 * Rollback an object to a self-managed snapshot
 *
 * The contents of the object will be the same as
 * when the snapshot was taken.
 *
 * @param io the pool in which the object is stored
 * @param oid the name of the object to rollback
 * @param snapid which snapshot to rollback to
 * @return 0 on success, negative error code on failure
 */
int rados_ioctx_selfmanaged_snap_rollback(rados_ioctx_t io, const char *oid, rados_snap_t snapid);

/**
 * Set the snapshot context for use when writing to objects
 *
 * This is stored in the io context, and applies to all future writes.
 *
 * @param io the io context to change
 * @param seq the newest snapshot sequence number for the pool
 * @param snaps array of snapshots in sorted by descending id
 * @param num_snaps how many snaphosts are in the snaps array
 * @return 0 on success, negative error code on failure.
 * -EINVAL is returned if snaps are not in descending order.
 */
int rados_ioctx_selfmanaged_snap_set_write_ctx(rados_ioctx_t io, rados_snap_t seq, rados_snap_t *snaps, int num_snaps);

/**
 * List all the ids of pool snapshots
 *
 * If the output array does not have enough space to fit all the
 * snapshots, -ERANGE is returned and the caller should retry with a
 * larger array.
 *
 * @param io the pool to read from
 * @param snaps where to store the results
 * @param maxlen the number of rados_snap_t that fit in the snaps array
 * @return number of snapshots on success, negative error code on failure.
 * -ERANGE is returned if the snaps array is too short.
 */
int rados_ioctx_snap_list(rados_ioctx_t io, rados_snap_t *snaps, int maxlen);

/**
 * Get the id of a pool snapshot
 *
 * @param io the pool to read from
 * @param name the snapshot to find
 * @param id where to store the result
 * @return 0 on success, negative error code on failure
 */
int rados_ioctx_snap_lookup(rados_ioctx_t io, const char *name, rados_snap_t *id);

/**
 * Get the name of a pool snapshot
 *
 * @param io the pool to read from
 * @param id the snapshot to find
 * @param name where to store the result
 * @param maxlen the size of the name array
 * @return 0 on success, negative error code on failure
 * @return -ERANGE if the name array is too small
 */
int rados_ioctx_snap_get_name(rados_ioctx_t io, rados_snap_t id, char *name, int maxlen);

/**
 * Find when a pool snapshot occurred
 *
 * @param io the pool the snapshot was taken in
 * @param id the snapshot to lookup
 * @param t where to store the result
 * @return 0 on success, negative error code on failure
 */
int rados_ioctx_snap_get_stamp(rados_ioctx_t io, rados_snap_t id, time_t *t);

/** @} Snapshots */

/**
 * @defgroup synch_io Synchronous I/O
 * Writes are replicated to a number of OSDs based on the
 * configuration of the pool they are in. These write functions block
 * until data is in memory on all replicas of the object they're
 * writing to - they are equivalent to doing the corresponding
 * asynchronous write, and the calling
 * rados_ioctx_wait_for_complete().  For greater data safety, use the
 * asynchronous functions and rados_aio_wait_for_safe().
 *
 * @{
 */

/**
 * Return the version of the last object read or written to.
 *
 * This exposes the internal version number of the last object read or
 * written via this io context
 *
 * @param io the io context to check
 * @return last read or written object version
 */
uint64_t rados_get_last_version(rados_ioctx_t io);

/**
 * Write data to an object
 *
 * @param io the io context in which the write will occur
 * @param oid name of the object
 * @param buf data to write
 * @param len length of the data, in bytes
 * @param off byte offset in the object to begin writing at
 * @return number of bytes written on success, negative error code on
 * failure
 */
int rados_write(rados_ioctx_t io, const char *oid, const char *buf, size_t len, uint64_t off);

/**
 * Write an entire object
 *
 * The object is filled with the provided data. If the object exists,
 * it is atomically truncated and then written.
 *
 * @param io the io context in which the write will occur
 * @param oid name of the object
 * @param buf data to write
 * @param len length of the data, in bytes
 * @return 0 on success, negative error code on failure
 */
int rados_write_full(rados_ioctx_t io, const char *oid, const char *buf, size_t len);

/**
 * Efficiently copy a portion of one object to another
 *
 * If the underlying filesystem on the OSD supports it, this will be a
 * copy-on-write clone.
 *
 * The src and dest objects must be in the same pg. To ensure this,
 * the io context should have a locater key set (see
 * rados_ioctx_locator_set_key).
 *
 * @param io the context in which the data is cloned
 * @param dst the name of the destination object
 * @param dst_off the offset within the destination object (in bytes)
 * @param src the name of the source object
 * @param src_off the offset within the source object (in bytes)
 * @param len how much data to copy
 * @return 0 on success, negative error code on failure
 */
int rados_clone_range(rados_ioctx_t io, const char *dst, uint64_t dst_off,
                      const char *src, uint64_t src_off, size_t len);

/**
 * Append data to an object
 *
 * @param io the context to operate in
 * @param oid the name of the object
 * @param buf the data to append
 * @param len length of buf (in bytes)
 * @return number of bytes written on success, negative error code on
 * failure
 */
int rados_append(rados_ioctx_t io, const char *oid, const char *buf, size_t len);

/**
 * Read data from an object
 *
 * The io context determines the snapshot to read from, if any was set
 * by rados_ioctx_set_snap_read.
 * 
 * @param io the context in which to perform the read
 * @param oid the name of the object to read from
 * @param buf where to store the results
 * @param len the number of bytes to read
 * @param off the offset to start reading from in the object
 * @return number of bytes read on success, negative error code on
 * failure
 */
int rados_read(rados_ioctx_t io, const char *oid, char *buf, size_t len, uint64_t off);

/**
 * Delete an object
 *
 * @note This does not delete any snapshots of the object.
 *
 * @param io the pool to delete the object from
 * @param oid the name of the object to delete
 * @return 0 on success, negative error code on failure
 */
int rados_remove(rados_ioctx_t io, const char *oid);

/**
 * Resize an object
 *
 * If this enlarges the object, the new area is logically filled with
 * zeroes. If this shrinks the object, the excess data is removed.
 *
 * @param io the context in which to truncate
 * @param oid the name of the object
 * @param size the new size of the object in bytes
 * @return 0 on success, negative error code on failure
 */
int rados_trunc(rados_ioctx_t io, const char *oid, uint64_t size);

/**
 * @defgroup xattrs Xattrs
 * Extended attributes are stored as extended attributes on the files
 * representing an object on the OSDs. Thus, they have the same
 * limitations as the underlying filesystem. On ext4, this means that
 * the total data stored in xattrs cannot exceed 4KB.
 *
 * @{
 */

/**
 * Get the value of an extended attribute on an object.
 *
 * @param io the context in which the attribute is read
 * @param o name of the object
 * @param name which extended attribute to read
 * @param buf where to store the result
 * @param len size of buf in bytes
 * @return 0 on success, negative error code on failure
 */
int rados_getxattr(rados_ioctx_t io, const char *o, const char *name, char *buf, size_t len);

/**
 * Set an extended attribute on an object.
 *
 * @param io the context in which xattr is set
 * @param o name of the object
 * @param name which extended attribute to set
 * @param buf what to store in the xattr
 * @param len the number of bytes in buf
 * @return 0 on success, negative error code on failure
 */
int rados_setxattr(rados_ioctx_t io, const char *o, const char *name, const char *buf, size_t len);

/**
 * Delete an extended attribute from an object.
 *
 * @param io the context in which to delete the xattr
 * @param o the name of the object
 * @param name which xattr to delete
 * @return 0 on success, negative error code on failure
 */
int rados_rmxattr(rados_ioctx_t io, const char *o, const char *name);

/**
 * Start iterating over xattrs on an object.
 *
 * @post iter is a valid iterator
 *
 * @param io the context in which to list xattrs
 * @param oid name of the object
 * @param iter where to store the iterator
 * @return 0 on success, negative error code on failure
 */
int rados_getxattrs(rados_ioctx_t io, const char *oid, rados_xattrs_iter_t *iter);

/**
 * Get the next xattr on the object
 *
 * @pre iter is a valid iterator
 *
 * @post name is the NULL-terminated name of the next xattr, and val
 * contains the value of the xattr, which is of length len. If the end
 * of the list has been reached, name and val are NULL, and len is 0.
 *
 * @param iter iterator to advance
 * @param name where to store the name of the next xattr
 * @param val where to store the value of the next xattr
 * @param len the number of bytes in val
 * @return 0 on success, negative error code on failure
 */
int rados_getxattrs_next(rados_xattrs_iter_t iter, const char **name,
			 const char **val, size_t *len);

/**
 * Close the xattr iterator.
 *
 * iter should not be used after this is called.
 *
 * @param iter the iterator to close
 */
void rados_getxattrs_end(rados_xattrs_iter_t iter);

/** @} Xattrs */

/**
 * Get object stats (size/mtime)
 *
 * TODO: when are these set, and by whom? can they be out of date?
 *
 * @param io ioctx
 * @param o object name
 * @param psize where to store object size
 * @param pmtime where to store modification time
 * @return 0 on success, negative error code on failure
 */
int rados_stat(rados_ioctx_t io, const char *o, uint64_t *psize, time_t *pmtime);

/**
 * Update tmap (trivial map)
 *
 * Do compound update to a tmap object, inserting or deleting some
 * number of records.  cmdbuf is a series of operation byte
 * codes, following by command payload.  Each command is a single-byte
 * command code, whose value is one of CEPH_OSD_TMAP_*.
 *
 *  - update tmap 'header'
 *    1 byte  = CEPH_OSD_TMAP_HDR
 *    4 bytes = data length (little endian)
 *    N bytes = data
 *
 *  - insert/update one key/value pair
 *    1 byte  = CEPH_OSD_TMAP_SET
 *    4 bytes = key name length (little endian)
 *    N bytes = key name
 *    4 bytes = data length (little endian)
 *    M bytes = data
 *
 *  - insert one key/value pair; return -EEXIST if it already exists.
 *    1 byte  = CEPH_OSD_TMAP_CREATE
 *    4 bytes = key name length (little endian)
 *    N bytes = key name
 *    4 bytes = data length (little endian)
 *    M bytes = data
 *
 *  - remove one key/value pair
 *    1 byte  = CEPH_OSD_TMAP_RM
 *    4 bytes = key name length (little endian)
 *    N bytes = key name
 *
 * Restrictions:
 *  - The HDR update must preceed any key/value updates.
 *  - All key/value updates must be in lexicographically sorted order
 *    in cmdbuf.
 *  - You can read/write to a tmap object via the regular APIs, but
 *    you should be careful not to corrupt it.  Also be aware that the
 *    object format may change without notice.
 *
 * @param io ioctx
 * @param o object name
 * @param cmdbuf command buffer
 * @param cmdbuflen command buffer length
 * @return 0 for success or negative error code
 */
int rados_tmap_update(rados_ioctx_t io, const char *o, const char *cmdbuf, size_t cmdbuflen);

/**
 * Store complete tmap (trivial map) object
 *
 * Put a full tmap object into the store, replacing what was there.
 *
 * The format of buf is:
 *    4 bytes - length of header (little endian)
 *    N bytes - header data
 *    4 bytes - number of keys (little endian)
 * and for each key,
 *    4 bytes - key name length (little endian)
 *    N bytes - key name
 *    4 bytes - value length (little endian)
 *    M bytes - value data
 *
 * @param io ioctx
 * @param o object name
 * @param buf buffer
 * @param buflen buffer length
 * @return 0 for success or negative error code
 */
int rados_tmap_put(rados_ioctx_t io, const char *o, const char *buf, size_t buflen);

/**
 * Fetch complete tmap (trivial map) object
 *
 * Read a full tmap object.  See rados_tmap_put() for the format the
 * data is returned in.  If the supplied buffer isn't big enough,
 * returns -ERANGE.
 *
 * @param io ioctx
 * @param o object name
 * @param buf buffer
 * @param buflen buffer length
 * @return 0 for success or negative error code
 */
int rados_tmap_get(rados_ioctx_t io, const char *o, char *buf, size_t buflen);

/**
 * Execute an OSD class method on an object
 *
 * The OSD has a plugin mechanism for performing complicated
 * operations on an object atomically. These plugins are called
 * classes. This function allows librados users to call the custom
 * methods. The input and output formats are defined by the class.
 * Classes in ceph.git can be found in src/cls_*.cc
 *
 * @param io the context in which to call the method
 * @param oid the object to call the method on
 * @param cls the name of the class
 * @param method the name of the method
 * @param in_buf where to find input
 * @param in_len length of in_buf in bytes
 * @param buf where to store output
 * @param out_len length of buf in bytes
 * @return For methods that return data, the length of the output, or
 * -ERANGE if out_buf does not have enough space to store it. For
 * methods that don't return data, the return value is
 * method-specific.
 */
int rados_exec(rados_ioctx_t io, const char *oid, const char *cls, const char *method,
	       const char *in_buf, size_t in_len, char *buf, size_t out_len);


/** @} Synchronous I/O */

/**
 * @defgroup asynch_io Asynchronous I/O
 * Read and write to objects without blocking.
 *
 * @{
 */

/**
 * @typedef rados_completion_t
 * Represents the state of an asynchronous operation - it contains the
 * return value once the operation completes, and can be used to block
 * until the operation is complete or safe.
 */
typedef void *rados_completion_t;

/**
 * @typedef rados_callback_t
 * Callbacks for asynchrous operations take two parameters:
 * - cb the completion that has finished
 * - arg application defined data made available to the callback function
 */
typedef void (*rados_callback_t)(rados_completion_t cb, void *arg);

/**
 * Constructs a completion to use with asynchronous operations
 *
 * The complete and safe callbacks correspond to operations being
 * acked and committed, respectively. The callbacks are called in
 * order of receipt, so the safe callback may be triggered before the
 * complete callback, and vice versa. This is affected by journalling
 * on the OSDs.
 *
 * TODO: more complete documentation of this elsewhere (in the RADOS docs?)
 *
 * @note Read operations only get a complete callback.
 * BUG: this should check for ENOMEM instead of throwing an exception
 *
 * @param cb_arg application-defined data passed to the callback functions
 * @param cb_complete the function to be called when the operation is
 * in memory on all relpicas
 * @param cb_safe the function to be called when the operation is on
 * stable storage on all replicas
 * @param pc where to store the completion
 * @return 0
 */
int rados_aio_create_completion(void *cb_arg, rados_callback_t cb_complete, rados_callback_t cb_safe,
				rados_completion_t *pc);

/**
 * Block until an operation completes
 *
 * This means it is in memory on all replicas.
 *
 * BUG: this should be void
 *
 * @param c operation to wait for
 * @return 0
 */
int rados_aio_wait_for_complete(rados_completion_t c);

/**
 * Block until an operation is safe
 *
 * This means it is on stable storage on all replicas.
 *
 * BUG: this should be void
 *
 * @param c operation to wait for
 * @return 0
 */
int rados_aio_wait_for_safe(rados_completion_t c);

/**
 * Has an asynchronous operation completed?
 *
 * @warning This does not imply that the complete callback has
 * finished
 *
 * @param c async operation to inspect
 * @return whether c is complete
 */
int rados_aio_is_complete(rados_completion_t c);

/**
 * Is an asynchronous operation safe?
 *
 * @warning This does not imply that the safe callback has
 * finished
 *
 * @param c async operation to inspect
 * @return whether c is safe
 */
int rados_aio_is_safe(rados_completion_t c);

/**
 * Get the return value of an asychronous operation
 *
 * The return value is set when the operation is complete or safe,
 * whichever comes first.
 *
 * @pre The operation is safe or complete
 *
 * BUG: complete callback may never be called when the safe
 * message is received before the complete message
 *
 * @param c async operation to inspect
 * @return return value of the operation (see sychronous version of
 * operation for expected values)
 */
int rados_aio_get_return_value(rados_completion_t c);

/**
 * Release a completion
 *
 * Call this when you no longer need the completion. It may not be
 * freed immediately if the operation is not acked and committed.
 *
 * @param c completion to release
 */
void rados_aio_release(rados_completion_t c);

/**
 * Write data to an object asynchronously
 *
 * Queues the write and returns.
 *
 * @param io the context in which the write will occur
 * @param oid name of the object
 * @param completion what to do when the write is safe and complete
 * @param buf data to write
 * @param len length of the data, in bytes
 * @param off byte offset in the object to begin writing at
 * @return 0 on success, -EROFS if the io context specifies a snap_seq
 * other than CEPH_NOSNAP
 */
int rados_aio_write(rados_ioctx_t io, const char *oid,
		    rados_completion_t completion,
		    const char *buf, size_t len, uint64_t off);

/**
 * Asychronously append data to an object
 *
 * Queues the append and returns.
 *
 * @param io the context to operate in
 * @param oid the name of the object
 * @param completion what to do when the append is safe and complete
 * @param buf the data to append
 * @param len length of buf (in bytes)
 * @return 0 on success, -EROFS if the io context specifies a snap_seq
 * other than CEPH_NOSNAP
 */
int rados_aio_append(rados_ioctx_t io, const char *oid,
		     rados_completion_t completion,
		     const char *buf, size_t len);

/**
 * Asychronously write an entire object
 *
 * The object is filled with the provided data. If the object exists,
 * it is atomically truncated and then written.
 * Queues the write_full and returns.
 *
 * @param io the io context in which the write will occur
 * @param oid name of the object
 * @param completion what to do when the write_full is safe and complete
 * @param buf data to write
 * @param len length of the data, in bytes
 * @return 0 on success, -EROFS if the io context specifies a snap_seq
 * other than CEPH_NOSNAP
 */
int rados_aio_write_full(rados_ioctx_t io, const char *oid,
			 rados_completion_t completion,
			 const char *buf, size_t len);

/**
 * Asychronously read data from an object
 *
 * The io context determines the snapshot to read from, if any was set
 * by rados_ioctx_set_snap_read.
 *
 * @note only the 'complete' callback of the completion will be called.
 *
 * @param io the context in which to perform the read
 * @param oid the name of the object to read from
 * @param completion what to do when the read is complete
 * @param buf where to store the results
 * @param len the number of bytes to read
 * @param off the offset to start reading from in the object
 * @return 0 on success, negative error code on failure
 */
int rados_aio_read(rados_ioctx_t io, const char *oid,
		   rados_completion_t completion,
		   char *buf, size_t len, uint64_t off);

/**
 * Block until all pending writes in an io context are safe
 *
 * This is not equivalent to calling rados_aio_wait_for_safe() on all
 * write completions, since this waits for the associated callbacks to
 * complete as well.
 *
 * BUG: always returns 0, should be void or accept a timeout
 *
 * @param io the context to flush
 * @return 0 on success, negative error code on failure
 */
int rados_aio_flush(rados_ioctx_t io);

/** @} Asynchronous I/O */

/**
 * @defgroup watch_notify Watch/Notify
 *
 * Watch/notify is a protocol to help communicate among clients. It
 * can be used to sychronize client state. All that's needed is a
 * well-known object name (for example, rbd uses the header object of
 * an image).
 *
 * Watchers register an interest in an object, and receive all
 * notifies on that object. A notify attempts to communicate with all
 * clients watching an object, and blocks on the notifier until each
 * client responds or a timeout is reached.
 *
 * See rados_watch() and rados_notify() for more details.
 *
 * @{
 */

/**
 * @typedef rados_watchcb_t
 *
 * Callback activated when a notify is received on a watched
 * object. Parameters are:
 * - opcode undefined
 * - ver version of the watched object
 * - arg application-specific data
 *
 * BUG: opcode is an internal detail that shouldn't be exposed
 */
typedef void (*rados_watchcb_t)(uint8_t opcode, uint64_t ver, void *arg);

/**
 * Register an interest in an object
 *
 * A watch operation registers the client as being interested in
 * notifications on an object. OSDs keep track of watches on
 * persistent storage, so they are preserved across cluster changes by
 * the normal recovery process. If the client loses its connection to
 * the primary OSD for a watched object, the watch will be removed
 * after 30 seconds. Watches are automatically reestablished when a new
 * connection is made, or a placement group switches OSDs.
 *
 * BUG: watch timeout should be configurable
 * BUG: librados should provide a way for watchers to notice connection resets
 *
 * @param io the pool the object is in
 * @param o the object to watch
 * @param ver expected version of the object
 * @param handle where to store the internal id assigned to this watch
 * @param watchcb what to do when a notify is received on this object
 * @param arg application defined data to pass when watchcb is called
 * @return 0 on success, negative error code on failure. -ERANGE is
 * returned, and the watch is not registered, if the version of the
 * object is greater than ver.
 */
int rados_watch(rados_ioctx_t io, const char *o, uint64_t ver, uint64_t *handle,
                rados_watchcb_t watchcb, void *arg);

/**
 * Unregister an interest in an object
 *
 * Once this completes, no more notifies will be sent to us for this
 * watch. This should be called to clean up unneeded watchers.
 *
 * @param io the pool the object is in
 * @param o the name of the watched object
 * @param handle which watch to unregister
 * @return 0 on success, negative error code on failure
 */
int rados_unwatch(rados_ioctx_t io, const char *o, uint64_t handle);

/**
 * Sychronously notify watchers of an object
 *
 * This blocks until all watchers of the object have received and
 * reacted to the notify, or a timeout is reached.
 *
 * BUG: the timeout is not changeable via the C API
 * BUG: the bufferlist is inaccessible in a rados_watchcb_t
 *
 * @param io the pool the object is in
 * @param o the name of the object
 * @param ver obsolete - just pass zero
 * @param buf data to send to watchers
 * @param buf_len length of buf in bytes
 * @return 0 on success, negative error code on failure
 */
int rados_notify(rados_ioctx_t io, const char *o, uint64_t ver, const char *buf, int buf_len);

/** @} Watch/Notify */

#ifdef __cplusplus
}
#endif

#endif
