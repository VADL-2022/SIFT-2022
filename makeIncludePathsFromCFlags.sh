#!/usr/bin/env bash

#echo "$NIX_CFLAGS_COMPILE" # Example: `-frandom-seed=n2ajxmanxs -isystem /nix/store/w6mp8b25lld3b7s0gvi709dfkw619s36-libcxx-7.1.0-dev/include -isystem /nix/store/9ga98pnqwjznb93vvgxx5j14137j7wcn-libcxxabi-7.1.0-dev/include -isystem /nix/store/9zgpq6jx0s1vf5jnh9659a26gp18asj6-compiler-rt-libc-7.1.0-dev/include -isystem /nix/store/4k27z2v6jqg6nwqdqwm04xq6p5agmag0-python3-3.9.6-env/include -isystem /nix/store/gzmldcqhy7scwriamp4jlvqzhm6s2kqg-opencv-4.5.2/include -isystem /nix/store/agv1803y52ln3lm7c44z5qbsi5arlyb6-libpng-apng-1.6.37-dev/include -isystem /nix/store/k3a8va25mnbb4kzbxd1r214cfk1yfsf1-zlib-1.2.11-dev/include -iframework /nix/store/v9n08h43mywykfvk93wa561sjwgkznp9-swift-corefoundation/Library/Frameworks -isystem /nix/store/w6mp8b25lld3b7s0gvi709dfkw619s36-libcxx-7.1.0-dev/include -isystem /nix/store/9ga98pnqwjznb93vvgxx5j14137j7wcn-libcxxabi-7.1.0-dev/include -isystem /nix/store/9zgpq6jx0s1vf5jnh9659a26gp18asj6-compiler-rt-libc-7.1.0-dev/include -isystem /nix/store/4k27z2v6jqg6nwqdqwm04xq6p5agmag0-python3-3.9.6-env/include -isystem /nix/store/gzmldcqhy7scwriamp4jlvqzhm6s2kqg-opencv-4.5.2/include -isystem /nix/store/agv1803y52ln3lm7c44z5qbsi5arlyb6-libpng-apng-1.6.37-dev/include -isystem /nix/store/k3a8va25mnbb4kzbxd1r214cfk1yfsf1-zlib-1.2.11-dev/include -iframework /nix/store/v9n08h43mywykfvk93wa561sjwgkznp9-swift-corefoundation/Library/Frameworks`

echo "$NIX_CFLAGS_COMPILE" | tr -s '[:blank:]' '[\n*]' | while IFS= read -r word; do
    if [ "$word" == "-isystem" ] || [ "$word" == "-iframework" ] || [[ "$word" =~ -frandom-seed=* ]] || [ -z "$word" ]; then
	: # Do nothing
    else
	echo "$word";
    fi
done
