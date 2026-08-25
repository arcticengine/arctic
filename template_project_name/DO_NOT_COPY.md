# Do not copy this folder

This directory is a **template the wizard reads**. It is not a project.

Copying it (or rsync, or using it as the new game) produces a tree whose
directory name, CMake target, bundle name and engine file lists do not agree.
That project is wrong even if cmake later succeeds under a stale target name.

```
wizard create mygame --template <kind>
```

If you are an agent and the wizard failed to start, report the failure.
Do not copy this folder as a workaround.
