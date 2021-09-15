This is a library for testing mainly my new ideas and maintaining and improving my some of my skills.

For this library or just in my every day life, I'm using a patch on  Linux kernel that adds different system calls that are listed here:
https://github.com/XutaxKamay/mylib/blob/master/src/custom_linux_syscalls.h

Now as why I'm not linking any gist for the patch:
It is because it would need to rewrite a lot of portion of the Linux code as I'm doing it in a dirty way right now.
My trick for now is to override the 'current' macro for getting the current task running.
