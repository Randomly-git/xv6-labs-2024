#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// 拼接路径，并在后面追加文件名
void fmtname(char *path, char *buf, char *name) {
  strcpy(buf, path);
  int len = strlen(path);
  if (path[len - 1] != '/') {
    buf[len++] = '/';
  }
  strcpy(buf + len, name);
}

void find(char *path, char *target) {
  char buf[512];
  int fd;
  struct dirent de;
  struct stat st;

  fd = open(path, 0);
  if (fd < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  // 如果是文件，检查是否是目标名
  if (st.type == T_FILE) {
    // 提取最后一段文件名
    char *p = path + strlen(path);
    while (p >= path && *p != '/') p--;
    p++;
    if (strcmp(p, target) == 0) {
      printf("%s\n", path);
    }
  }

  // 如果是目录，递归查找
  if (st.type == T_DIR) {
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0)
        continue;

      // 忽略 "." 和 ".."
      if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;

      memset(buf, 0, sizeof(buf));
      fmtname(path, buf, de.name);

      if (stat(buf, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", buf);
        continue;
      }

      // 递归进入子项
      if (st.type == T_DIR || st.type == T_FILE) {
        find(buf, target);
      }
    }
  }

  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(2, "Usage: find <path> <target>\n");
    exit(1);
  }

  find(argv[1], argv[2]);
  exit(0);
}
