#pragma once

struct Visible {};       					// tag — entity should be drawn
struct DirtyCameraProjection {}; 	// tag — ProjectionMatrix needs recompute

struct PlayerTag {};							// tag - for the players

struct DirtyTransform {}; 				// tag - transform needs recompute
// // e.g. in your input/physics system
// registry.patch<Transform>(entity, [&](Transform& t) {
//     t.position += delta;
// });
// registry.emplace_or_replace<DirtyTransform>(entity);
