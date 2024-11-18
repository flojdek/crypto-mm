IN PROGRESS...

![Diagram](./crypto-mm.drawio.png)

# Improvements

* Not production ready yet - a lot of small details to polish in terms of many aspects:
  * Graceful shutdown.
  * State management.
  * Logging.
  * etc.
* For now just the symbol is one and hardcoded and the same there's just one bid and one ask. For something like grid
  trading you would usually create different points on the grid and a kind of "ladder" but that's another story.
* Generate unit tests as separate binaries and don't include them in the main binary: smaller binary, better locality, less risk.
* For any integration tests it is usually better and safer for those to live outside of the repository. Unless these are
  kind of in between where we mock some stuff and we use something from real world but those definitely should not be
  compiled into the main binary.
* The std::cout obviously cannot stay there as that is not thread-safe and slow so will need some third-party async
  logger or implement one myself. Have to be very careful with logging in general as that is slow, pushing KPIs is
  usually a better idea.
* The number of threads is strictly controlled - one thread per client - one thread per the trading engine so we would
  do allocation of threads to CPUs and on any machine that we're running we would need to in general design the CPUs
  allocation. It would look sth along the lines of assuming 32 cores let's say:
    Common CPU Set: CPUs 0-7:
    - For system operations, background tasks, and other general processes.
    Isolated CPUs: CPUs 8-23:
    - Reserved for high-priority threads. Isolated using isolcpus.
    IRQ Handling CPUs: CPUs 24-31:
    - Reserved for interrupt handling to ensure isolated CPUs are not disrupted by IRQs.
* When we get data from clients and push data around our internal queues / system we probably want to work with the
  smallest, most efficient data possible.
  So I would imagine whatever we get from clients we convert to SBE or Protobuf or other binary format of our own so it
  flows through the system ASAP.
  I mean we would need to test this claim as that conversion needs to be much faster and happening at a much higher
  throughput than our system is intended to work.
  Obviously we would not pass things like side and state and other enums as strings...
* Add graceful shutdowns - we can only shutdown properly once all messages are drained from queues.
* Add recovery on startup - we have to recover state from the Exchange on startup because otherwise we're "blind".
  Ideally we would have cancel on disconnect but not every exchange supports that.