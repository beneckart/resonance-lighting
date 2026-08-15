import fs from "node:fs/promises";
import path from "node:path";
import { SpreadsheetFile, Workbook } from "@oai/artifact-tool";

const outputDir = path.resolve("ops/bench/solenoid-comparison-2026-07-25");
await fs.mkdir(outputDir, { recursive: true });
const workbook = Workbook.create();
const sheet = workbook.worksheets.add("Solenoid Comparison");
sheet.showGridLines = false;

sheet.getRange("A1:D1").merge();
sheet.getRange("A1").values = [["Solenoid Bench Comparison"]];
sheet.getRange("A2:D2").merge();
sheet.getRange("A2").values = [["Editable summary of the three solenoids tested with the Korad supply and capacitor bank."]];

sheet.getRange("A4:D10").values = [
  ["Property", "HS-0730B", "HS-0530B", "X003801GWR 5V solenoid"],
  ["Voltage", "6 V", "6 V", "5 V"],
  ["Ohms", "~3.76 ohm measured", "~1.72 ohm measured", "~7 ohm spec"],
  ["Stroke len", "10 mm", "10 mm", "~4.5-5 mm"],
  ["1 cap results", "Not tested with cap", "Good/satisfying snap at 6 V with 22,000 uF", "Not tested"],
  ["2 cap results", "Not tested with cap", "Charged/tested, but no final result recorded", "Tested at 5 V and 6 V, but no final result recorded"],
  ["Notes", "Moved fully at 100 ms direct; 50 ms did not show much movement", "Preferred: lower resistance, bigger kick; full throw at 50 ms direct", "Moved some on 5 V / 0.75 A / 100 ms direct"],
];

sheet.getRange("A1:D1").format = {
  fill: "#1F4E79",
  font: { bold: true, color: "#FFFFFF", size: 16 },
  horizontalAlignment: "center",
  verticalAlignment: "center",
};
sheet.getRange("A2:D2").format = {
  fill: "#D9EAF7",
  font: { italic: true, color: "#1F2937" },
  horizontalAlignment: "center",
  verticalAlignment: "center",
};
sheet.getRange("A4:D4").format = {
  fill: "#305496",
  font: { bold: true, color: "#FFFFFF" },
  horizontalAlignment: "center",
  verticalAlignment: "center",
};
sheet.getRange("A5:A10").format = {
  fill: "#EAF2F8",
  font: { bold: true, color: "#1F2937" },
};
sheet.getRange("B5:D10").format = {
  fill: "#FFFFFF",
  font: { color: "#111827" },
  wrapText: true,
  verticalAlignment: "top",
};
sheet.getRange("A4:D10").format.borders = {
  preset: "all",
  style: "thin",
  color: "#B7C9D6",
};
sheet.getRange("A1:D2").format.borders = {
  preset: "outside",
  style: "medium",
  color: "#1F4E79",
};

sheet.getRange("A1:D1").format.rowHeight = 28;
sheet.getRange("A2:D2").format.rowHeight = 24;
sheet.getRange("A4:D4").format.rowHeight = 24;
sheet.getRange("A5:D10").format.rowHeight = 42;
sheet.getRange("A:A").format.columnWidth = 18;
sheet.getRange("B:D").format.columnWidth = 34;
sheet.freezePanes.freezeRows(4);

const inspect = await workbook.inspect({
  kind: "table",
  sheetId: "Solenoid Comparison",
  range: "A1:D10",
  include: "values,formulas",
  tableMaxRows: 12,
  tableMaxCols: 5,
});
console.log(inspect.ndjson);

const errors = await workbook.inspect({
  kind: "match",
  searchTerm: "#REF!|#DIV/0!|#VALUE!|#NAME\\?|#N/A",
  options: { useRegex: true, maxResults: 50 },
  summary: "final formula error scan",
});
console.log(errors.ndjson);

const preview = await workbook.render({
  sheetName: "Solenoid Comparison",
  range: "A1:D10",
  scale: 1,
  format: "png",
});
await fs.writeFile(path.join(outputDir, "solenoid-comparison-preview.png"), new Uint8Array(await preview.arrayBuffer()));

const output = await SpreadsheetFile.exportXlsx(workbook);
await output.save(path.join(outputDir, "solenoid-comparison.xlsx"));
