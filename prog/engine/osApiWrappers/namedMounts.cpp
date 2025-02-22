#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_rwLock.h>
#include <EASTL/hash_map.h>
#include <EASTL/string.h>
#if _TARGET_PC_WIN | _TARGET_XBOX
#include <malloc.h>
#elif defined(__GNUC__)
#include <stdlib.h>
#endif
#include <debug/dag_debug.h>

static eastl::hash_map<eastl::string, eastl::string> named_mounts; // Note: intentionally no ska, since we return pointers to string
                                                                   // data
static OSReadWriteLock named_mounts_rwlock;

void dd_set_named_mount_path(const char *mount_name, const char *path_to)
{
  if (mount_name[0] == '%')
    mount_name++;
  if (path_to)
  {
    G_ASSERTF_RETURN(strlen(mount_name) < MAX_MOUNT_NAME_LEN, , "too long mount_name=%s", mount_name);
    for (const char *m = mount_name; *m; m++)
      G_ASSERTF_RETURN((*m >= 'a' && *m <= 'z') || (*m >= 'A' && *m <= 'Z') || (*m >= '0' && *m <= '9') || *m == '_', ,
        "invalid mount_name=%s", mount_name);
    if (*path_to == '.' && *(path_to + 1) == '\0')
      path_to = "";

    ScopedLockWriteTemplate<OSReadWriteLock> lock(named_mounts_rwlock);
    named_mounts[mount_name] = path_to;
  }
  else
  {
    ScopedLockWriteTemplate<OSReadWriteLock> lock(named_mounts_rwlock);
    auto it = named_mounts.find_as(mount_name);
    if (it != named_mounts.end())
      named_mounts.erase(it);
  }
}

const char *dd_get_named_mount_path(const char *mount_name, int mount_name_len)
{
  ScopedLockReadTemplate<OSReadWriteLock> lock(named_mounts_rwlock);
  if (mount_name_len >= 0 && mount_name[mount_name_len])
  {
    char *p = (char *)alloca(mount_name_len + 1);
    memcpy(p, mount_name, mount_name_len);
    p[mount_name_len] = '\0';
    mount_name = p;
  }
  auto it = named_mounts.find_as(mount_name);
  return (it != named_mounts.end()) ? it->second.c_str() : nullptr;
}

bool dd_check_named_mount_in_path_valid(const char *fpath)
{
  if (fpath && *fpath == '%')
  {
    if (const char *p = strchr(fpath + 1, '/'))
      return dd_get_named_mount_path(fpath + 1, p - fpath - 1) != nullptr;
    else
      return dd_get_named_mount_path(fpath + 1) != nullptr;
  }
  return true;
}

const char *dd_resolve_named_mount_in_path(const char *fpath, const char *&mnt_path)
{
  if (fpath && *fpath == '%')
  {
    if (const char *p = strchr(fpath + 1, '/'))
    {
      if ((mnt_path = dd_get_named_mount_path(fpath + 1, p - fpath - 1)) != nullptr)
        return *mnt_path ? p : p + 1;
      logerr("named mount <%.*s> not set for %s", p - fpath - 1, fpath + 1, fpath);
    }
    else
    {
      if ((mnt_path = dd_get_named_mount_path(fpath + 1)) != nullptr)
        return "";
      logerr("named mount <%.*s> not set", fpath + 1);
    }
  }
  mnt_path = "";
  return fpath;
}

void dd_dump_named_mounts()
{
  ScopedLockReadTemplate<OSReadWriteLock> lock(named_mounts_rwlock);
#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
  debug("registered %d named mount(s)%s", named_mounts.size(), named_mounts.size() > 0 ? ":" : "");
  for (auto &it : named_mounts)
    debug("  %%%s = %s", it.first.c_str(), it.second.c_str());
#endif
}

#define EXPORT_PULL dll_pull_osapiwrappers_namedMounts
#include <supp/exportPull.h>
