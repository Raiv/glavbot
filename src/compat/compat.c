//#ifdef __GNUC__
//#include <features.h>
//#if !__GNUC_PREREQ(2,17)
//
//#define _GNU_SOURCE
//#include <sys/socket.h>
//#include <unistd.h>
//#include <sys/syscall.h>
//
//int
//sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen, unsigned int flags)
//{
//    long res = -1;
//    res = syscall(SYS_sendmsg, sockfd, msgvec, vlen, flags);
//    return res;
//}
//
//#endif  // __GNUC_PREREQ
//#endif  // __GNUC__