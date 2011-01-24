char *GET(const char *path, size_t *outsz)
{
  static char buf[512], *ptr, *rptr;
  size_t sz;
  int rsz;
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  // servaddr.sin_len = sizeof(servaddr);
  servaddr.sin_family = AF_INET;
#if __POWERPC__
  servaddr.sin_addr.s_addr = 0x4c490d03;
#else
  servaddr.sin_addr.s_addr = 0x030d494c;
#endif
  servaddr.sin_port = htons(80);
  if (connect (sock, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
  {
    return NULL;
  }
  sprintf(buf, "GET %s HTTP/1.0\r\nHost: pkg-distro.us\r\n\r\n", path);
  write(sock, buf, strlen(buf));

  ptr = buf;
#ifdef __POWERPC__
  for(ptr = buf; ptr - buf <= 4 || *((uint32_t *)(ptr - 4)) != 0x0d0a0d0a; read(sock, ptr++, 1));
#else
  for(ptr = buf; ptr - buf <= 4 || *((uint32_t *)(ptr - 4)) != 0x0a0d0a0d; read(sock, ptr++, 1));
#endif
  *ptr = 0;
  ptr = strstr(buf, "Location:");
  if (ptr)
  {
    ptr += 9;
    while (*ptr == ' ') ptr++;
    *strchr (ptr, '\r') = 0;
    if (strchr (ptr, '\n'))
      *strchr (ptr, '\n') = 0;
    char *dup = strdup(ptr);
    char *out = GET(dup, outsz);
    free (dup);
    return out;
  }
  ptr = strstr(buf, "Content-Length:");
  if (!ptr)
  {
    return NULL;
  }
  ptr += strlen("Content-Length: ");
  sz = atoi(ptr);
  if (outsz)
    *outsz = sz;
  ptr = (char *) malloc(sz + 1);
  ptr[sz] = 0;
  for (rptr = ptr; sz > 0; rptr += rsz)
  {
    rsz = read(sock, rptr, sz);
    if (rsz <= 0)
      break;
    sz -= rsz;
  }
  close(sock);
  return ptr;
}


