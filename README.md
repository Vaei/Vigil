# Vigil <img align="right" width=128, height=128 src="https://github.com/Vaei/Vigil/blob/main/Resources/Icon128.png">

> [!IMPORTANT]
> **Focus Targeting System**
> <br>Robust, Data-Driven, Asynchronous
> <br>With optional Net-Prediction
> <br>And its **FREE!**

> [!NOTE]
> **Vigil determines what the player is looking at**
> <br>What they want to interact with
> <br>Which enemy they want to lock onto
> <br>Which ally they want to auto-target with their healing spell

> [!WARNING]
> [Optionally download the pre-compiled binaries here](https://github.com/Vaei/Vigil/wiki/How-to-Use)
> <br>Install this as a project plugin, not an engine plugin
> <br>The content is not necessary, [reproducing it is very simple](https://github.com/Vaei/Vigil/wiki/Content-Creation)

> [!TIP]
> Suitable for both singleplayer and multiplayer games
> <br>Supports UE5.4+

> [!CAUTION]
> Vigil is currently in beta
> <br>There are many non-standard setups that have not been fully tested
> <br>Vigil has not been tested at scale in production
> <br>Vigil's net prediction has not been widely tested, esp. the workflow -- feedback wanted

## Watch Me

> [!TIP]
> [Showcase Video](https://youtu.be/fldDauZwYT8)

## How to Use
> [!IMPORTANT]
> [Read the Wiki to Learn How to use Vigil](https://github.com/Vaei/Vigil/wiki/How-to-Use)

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

## Changelog

### 1.4.2
* Add Fallback rotation sources for when InputVector or Velocity produce zero vectors

### 1.4.1
* Add InputVector and Velocity as rotation sources for target selection
	* Useful for top-down games that are pawn-relevant instead of camera-relevant
* Added helpers for cleaner code `UVigilStatics::GetActorFromVigilResult(Result);` and `UVigilStatics::GetComponentFromVigilResult`
	* Was really messy having to break the hit result structs in BP

### 1.4.0
* Fixed bug where destroyed actors were not resulting in events broadcasting

### 1.3.4
* Fixed category for VigilTargeting

### 1.3.3
* Change Failsafe Timer to Weak Lambda because OnDestroy not being called at correct point in lifecycle after UEngine::Browse (`open map`)

### 1.3.2
* Fix `UVigilScanTask::GetOwnerNetMode()` checks

### 1.3.1
* Replace native ptr with TObjectPtr

### 1.3.0
* Due to Engine bug where Targeting Subsystem loses all requests when another client joins, a failsafe has been added
* Fixed SourceActor Controller not finding PlayerController in `UVigilTargetingStatics::GetSourceLocation`
* Debug logging now identifies which client is logging

### 1.2.3
* Mark scan task as neither simulated nor ticking
* Mark Vigil category as Important to sort to top of details panel

### 1.2.2
* Fixed gating target results by unique actor, now multiple components on the same actor are valid focuses
* HitResult ImpactPoint now outputs the component location instead of actor
	* Debug drawing uses this, so it will correctly show the hit component location instead of actor now
* Fix out of order bAwaitingCallback flag and asynctaskdata being added for immediate trace post net sync, though it wasn't causing any issue


### 1.2.1
* Exposed ErrorWaitDelay on task node for advanced control and optimization
* Corrected UPROPERTY specificers

### 1.2.0
* Removed dependency on PlayerController
	* Allows for use on AIController, however this is untested

### 1.1.0
* Net prediction Workflow [see Wiki](https://github.com/Vaei/Vigil/wiki/Net-Prediction)
* Add `FindVigilComponentForActor` to save a BP cast

### 1.0.0
* Initial Release