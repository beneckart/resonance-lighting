/** Live aggregate of the tree's reported colour, written by TreeLights each
 *  frame and read by the GoboFloor spotlight so the ground PROJECTION is tinted
 *  by what the tree is actually doing — "real coloured shapes on the ground,
 *  not circles" (Elliot). A plain mutable object on purpose: no React
 *  subscription, no re-renders — both components touch it inside useFrame. */
export const groundTint = { r: 1, g: 0.88, b: 0.69, level: 0.5 };

/** Smooth the aggregate toward (r,g,b,level) with slew factor k (0..1). */
export function easeGroundTint(r: number, g: number, b: number, level: number, k: number) {
  groundTint.r += (r - groundTint.r) * k;
  groundTint.g += (g - groundTint.g) * k;
  groundTint.b += (b - groundTint.b) * k;
  groundTint.level += (level - groundTint.level) * k;
}
