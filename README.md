# Vigil <img align="right" width=128, height=128 src="https://github.com/Vaei/Vigil/blob/main/Resources/Icon128.png">

> [!CAUTION]
> This plugin is not ready at all
> <br>You should not use it yet

> [!IMPORTANT]
> **Gameplay Focus System**
> <br>Robust Data-Driven Asynchronous Focus Targeting
> <br>With optional Net-Prediction
> <br>And its **FREE!**

> [!WARNING]
> Use `git clone` instead of downloading as a zip, or you will not receive content
> <br>Install this as a project plugin, not an engine plugin
> <br>The content is not necessary, reproducing it is very simple

> [!TIP]
> Suitable for both singleplayer and multiplayer games
> <br>Supports UE5.4+

> [!CAUTION]
> Vigil is currently in beta
> <br>There are many non-standard setups that have not been fully tested
> <br>Vigil has not been tested at scale in production
> <br>Vigil's net prediction has not been widely tested, esp. the workflow -- feedback wanted

## Features

### Modular Data-Driven Setup

Very easy to customize, modify, and extend behaviours without writing any C++ or modifying the base plugin. Vigil is built on top of the Targeting System plugin.

### Quick Setup

Vigil can be setup in mere minutes with usable results drawn to screen.

### Multiple Focus Channels

Not a single-use focusing system, add as many focus channels as you need. Vigil will tell you what ally, and enemy, and interactable we're focused on separately.

### Robust Results

Vigil provides the results via a callback that you can assign to from anywhere. These are already sorted for you, so the current focus is the first index, but you have access to all options as well as their scoring for visual purposes if you need it.

### Optional Net Prediction

Run the scan ability as `Local Only` or set to `Local Predicted` if you need predicted focus results!

### Logging and Debugging

Vigil includes verbose logging that will allow you to see every stage of the focusing system to make diagnosing issues easy. There are debugging tools to visualize what is happening.

TODO: Notes

These are cliff notes for functionality

Built on TargetingSystem

Multiple focus channels (targeting presets)
	So you could focus on interactables, enemy targets, friendly targets, all separately

Presents weighted results instead of only the final result for optional visualization and extendable future processing

Consider setting scan ability to local only instead of local predicted so only client updates the focus and saves server perf

remove gas dependency tips i.e. change task remove ability run from PC

https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-targeting-system-debugging-in-unreal-engine

Set collision channel to custom channel

Hit result contains:
	actor via hitobjecthandle
	component
	impactpoint and location (identical)
	bblockinghit (but no bstartpenetrating)
	tracestart
	item
	distance
	normal -- source rotation, i.e. direction we are looking in for score comparison
	time -- the greatest possible angle difference, helpful for normalizing the angle to target for scoring (source component just gets sphere radius, not useful)  UVigilTargetingStatics::GetAngleToVigilTarget
	penetrationdepth - the greatest possible distance, as above  UVigilTargetingStatics::GetDistanceToVigilTarget

don't expect normalized distance to be useful on it's own, it is normalizing between some extreme approximations, do some remapping/clamping to get something you can work with for your use-case

first result always the primary focus, due to sorting

## Changelog

### 1.0.0
* Initial Release