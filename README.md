# Vigil <img align="right" width=128, height=128 src="https://github.com/Vaei/Vigil/blob/main/Resources/Icon128.png">

> [!CAUTION]
> This plugin is not ready at all
> <br>You should not use it yet

> [!IMPORTANT]
> **Gameplay Focusing System**
> <br>TODO
> <br>Pairs with the [Grasp](https://github.com/Vaei/Grasp) Interaction System
> <br>And its **Free!**

> [!WARNING]
> Use `git clone` instead of downloading as a zip, or you will not receive content
> <br>Install this as a project plugin, not an engine plugin

> [!TIP]
> Suitable for both singleplayer and multiplayer games
> <br>Supports UE5.5+


TODO: Notes

These are cliff notes for functionality

Built on TargetingSystem

Multiple focus channels (targeting presets)
	So you could focus on interactables, enemy targets, friendly targets, all separately

Presents weighted results instead of only the final result for optional visualization and extendable future processing

Consider setting scan ability to local only instead of local predicted so only client updates the focus and saves server perf

`Log LogVigil Verbose` / VeryVerbose

`p.Vigil.Debug.Draw`??

https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-targeting-system-debugging-in-unreal-engine

Set collision channel to custom channel

ts.debug.EnableTargetingDebugging REPLACED WITH p.Vigil.Selection.Debug , p.Vigil.Filter.Debug , 

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