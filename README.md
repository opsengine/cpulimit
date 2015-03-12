CPULIMIT
========

Cpulimit is a tool which limits the CPU usage of a process (expressed in percentage, not in CPU time). It is useful to control batch jobs, when you don't want them to eat too many CPU cycles. The goal is prevent a process from running for more than a specified time ratio. It does not change the nice value or other scheduling priority settings, but the real CPU usage. Also, it is able to adapt itself to the overall system load, dynamically and quickly.
The control of the used CPU amount is done sending SIGSTOP and SIGCONT POSIX signals to processes.
All the children processes and threads of the specified process will share the same percentage of CPU.

Developed by Angelo Marletta.
Please send your feedback, bug reports, feature requests or just thanks.


Get the latest source code
--------------------------

The latest available code is here:

https://github.com/opsengine/cpulimit


Install instructions
--------------------

On Linux/OS X:

    $ make
    # cp src/cpulimit /usr/bin

On FreeBSD:

    $ gmake
    # cp src/cpulimit /usr/bin

Run unit tests:

    $ ./tests/process_iterator_test


Contributions
-------------

You are welcome to contribute to cpulimit with bugfixes, new features, or support for a new OS.
If you want to submit a pull request, please do it on the branch develop and make sure all tests pass.
