#pragma once

struct Visible {};       					// tag — entity should be drawn
struct DirtyCameraTransform {}; 	// tag — WorldMatrix needs recompute TODO: Do I really need this? No real perf benefit, just make sure movement works.
struct DirtyCameraProjection {}; 	// tag — ProjectionMatrix needs recompute
