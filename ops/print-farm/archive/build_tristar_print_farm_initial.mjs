// Historical initial generator. The live Google Sheet received later manual edits.
import fs from "node:fs/promises";
import path from "node:path";
import { SpreadsheetFile, Workbook } from "@oai/artifact-tool";

const outputDir = path.resolve("outputs/019f7055-c657-7732-9bcb-a09c90913eec");
await fs.mkdir(outputDir, { recursive: true });

const workbook = Workbook.create();
workbook.comments.setSelf({ displayName: "Codex" });

const tracker = workbook.worksheets.add("Print Tracker");
const summary = workbook.worksheets.add("Summary");
const lists = workbook.worksheets.add("Lists");

const maxRows = 200;
const startRow = 2;
const endRow = startRow + maxRows - 1;

const headers = [
  "Priority",
  "Part Name",
  "Part Qty",
  "Material Type",
  "Material Color",
  "Material Qty (mg)",
  "Print Time Est (Hr)",
  "Total Print Time Estimate",
  "Name of person printing",
  "Qty Printed",
  "Time Left",
  "Complete",
];

tracker.getRange("A1:L1").values = [headers];

const seedRows = [
  ["High", "Hat enclosure top", 12, "PETG", "Black", 78000, 5.5, null, "Steve", 2, null, null],
  ["High", "Battery cradle", 12, "PETG", "Black", 24000, 1.8, null, "Jimmy", 0, null, null],
  ["Medium", "Gobo test disc", 20, "PLA", "Matte White", 9500, 0.75, null, "Adam", 6, null, null],
  ["Low", "Cable strain relief", 30, "TPU", "Black", 4200, 0.4, null, "Steve", 30, null, null],
];
tracker.getRange("A2:L5").values = seedRows;

tracker.getRange(`H${startRow}`).formulas = [[`=IF(OR(C${startRow}="",G${startRow}=""),"",C${startRow}*G${startRow})`]];
tracker.getRange(`H${startRow}:H${endRow}`).fillDown();
tracker.getRange(`K${startRow}`).formulas = [[`=IF(OR(C${startRow}="",G${startRow}=""),"",MAX(C${startRow}-N(J${startRow}),0)*G${startRow})`]];
tracker.getRange(`K${startRow}:K${endRow}`).fillDown();
tracker.getRange(`L${startRow}`).formulas = [[`=IF(C${startRow}="","",IF(N(J${startRow})>=C${startRow},"Yes","No"))`]];
tracker.getRange(`L${startRow}:L${endRow}`).fillDown();

tracker.tables.add(`A1:L${endRow}`, true, "PrintFarmTracker");
tracker.freezePanes.freezeRows(1);
tracker.showGridLines = false;

tracker.getRange("A1:L1").format = {
  fill: "#1F4E79",
  font: { bold: true, color: "#FFFFFF" },
  wrapText: true,
  horizontalAlignment: "center",
  verticalAlignment: "middle",
};
tracker.getRange("A1:L1").format.rowHeightPx = 42;
tracker.getRange(`A2:L${endRow}`).format = {
  font: { color: "#1F2937" },
  borders: { preset: "insideHorizontal", style: "thin", color: "#E5E7EB" },
  verticalAlignment: "middle",
};
tracker.getRange(`A2:B${endRow}`).format.horizontalAlignment = "left";
tracker.getRange(`D2:E${endRow}`).format.horizontalAlignment = "left";
tracker.getRange(`I2:I${endRow}`).format.horizontalAlignment = "left";
tracker.getRange(`C2:C${endRow}`).format.numberFormat = "#,##0";
tracker.getRange(`F2:F${endRow}`).format.numberFormat = "#,##0";
tracker.getRange(`G2:H${endRow}`).format.numberFormat = "#,##0.0";
tracker.getRange(`J2:J${endRow}`).format.numberFormat = "#,##0";
tracker.getRange(`K2:K${endRow}`).format.numberFormat = "#,##0.0";
tracker.getRange(`L2:L${endRow}`).format.horizontalAlignment = "center";

const widths = {
  A: 90, B: 190, C: 80, D: 115, E: 125, F: 120,
  G: 120, H: 150, I: 155, J: 95, K: 105, L: 90,
};
for (const [col, width] of Object.entries(widths)) {
  tracker.getRange(`${col}:${col}`).format.columnWidthPx = width;
}

tracker.getRange(`A2:A${endRow}`).dataValidation = {
  rule: { type: "list", values: ["High", "Medium", "Low"] },
};
tracker.getRange(`D2:D${endRow}`).dataValidation = {
  rule: { type: "list", formula1: "Lists!$A$2:$A$8" },
};
tracker.getRange(`I2:I${endRow}`).dataValidation = {
  rule: { type: "list", formula1: "Lists!$C$2:$C$4" },
};
tracker.getRange(`C2:C${endRow}`).dataValidation = {
  rule: { type: "whole", operator: "between", formula1: 0, formula2: 100000 },
};
tracker.getRange(`F2:H${endRow}`).dataValidation = {
  rule: { type: "decimal", operator: "greaterThanOrEqual", formula1: 0 },
};
tracker.getRange(`J2:J${endRow}`).dataValidation = {
  rule: { type: "whole", operator: "between", formula1: 0, formula2: 100000 },
};

tracker.getRange(`L2:L${endRow}`).conditionalFormats.add("cellIs", {
  operator: "equal",
  formula: '"Yes"',
  format: { fill: "#D9EAD3", font: { bold: true, color: "#166534" } },
});
tracker.getRange(`L2:L${endRow}`).conditionalFormats.add("cellIs", {
  operator: "equal",
  formula: '"No"',
  format: { fill: "#FCE4D6", font: { color: "#9A3412" } },
});
tracker.getRange(`A2:A${endRow}`).conditionalFormats.add("containsText", {
  text: "High",
  format: { fill: "#F4CCCC", font: { bold: true, color: "#7F1D1D" } },
});
tracker.getRange(`A2:A${endRow}`).conditionalFormats.add("containsText", {
  text: "Medium",
  format: { fill: "#FFF2CC", font: { color: "#7A4F01" } },
});
tracker.getRange(`A2:A${endRow}`).conditionalFormats.add("containsText", {
  text: "Low",
  format: { fill: "#D9EAD3", font: { color: "#166534" } },
});

workbook.comments.addThread({ cell: tracker.getRange("F1") }, "Enter material used per part in milligrams.");
workbook.comments.addThread({ cell: tracker.getRange("G1") }, "Enter estimated print time per part in hours.");
workbook.comments.addThread({ cell: tracker.getRange("H1") }, "Formula: Part Qty multiplied by Print Time Est.");
workbook.comments.addThread({ cell: tracker.getRange("K1") }, "Formula: remaining quantity multiplied by Print Time Est.");
workbook.comments.addThread({ cell: tracker.getRange("L1") }, "Formula returns Yes when Qty Printed is greater than or equal to Part Qty.");

summary.showGridLines = false;
summary.getRange("A1:F1").merge();
summary.getRange("A1:F1").values = [["Tri Star Print Farm"]];
summary.getRange("A1:F1").format = {
  fill: "#1F4E79",
  font: { bold: true, color: "#FFFFFF", size: 16 },
  horizontalAlignment: "center",
  verticalAlignment: "middle",
};
summary.getRange("A1:F1").format.rowHeightPx = 36;

summary.getRange("A3:B9").values = [
  ["Metric", "Value"],
  ["Total part quantity", null],
  ["Total qty printed", null],
  ["Remaining quantity", null],
  ["Total print hours", null],
  ["Hours left", null],
  ["Completion %", null],
];
summary.getRange("B4:B9").formulas = [
  [`=SUM('Print Tracker'!C${startRow}:C${endRow})`],
  [`=SUM('Print Tracker'!J${startRow}:J${endRow})`],
  [`=MAX(B4-B5,0)`],
  [`=SUM('Print Tracker'!H${startRow}:H${endRow})`],
  [`=SUM('Print Tracker'!K${startRow}:K${endRow})`],
  [`=IF(B4=0,0,B5/B4)`],
];
summary.getRange("A3:B3").format = {
  fill: "#D9EAF7",
  font: { bold: true, color: "#1F2937" },
};
summary.getRange("A3:B9").format.borders = { preset: "all", style: "thin", color: "#D1D5DB" };
summary.getRange("B4:B8").format.numberFormat = "#,##0.0";
summary.getRange("B4:B6").format.numberFormat = "#,##0";
summary.getRange("B9").format.numberFormat = "0.0%";

summary.getRange("D3:F7").values = [
  ["Printer", "Assigned Qty", "Hours Left"],
  ["Steve", null, null],
  ["Jimmy", null, null],
  ["Adam", null, null],
  ["Unassigned", null, null],
];
summary.getRange("E4:F7").formulas = [
  [`=SUMIF('Print Tracker'!I${startRow}:I${endRow},D4,'Print Tracker'!C${startRow}:C${endRow})`, `=SUMIF('Print Tracker'!I${startRow}:I${endRow},D4,'Print Tracker'!K${startRow}:K${endRow})`],
  [`=SUMIF('Print Tracker'!I${startRow}:I${endRow},D5,'Print Tracker'!C${startRow}:C${endRow})`, `=SUMIF('Print Tracker'!I${startRow}:I${endRow},D5,'Print Tracker'!K${startRow}:K${endRow})`],
  [`=SUMIF('Print Tracker'!I${startRow}:I${endRow},D6,'Print Tracker'!C${startRow}:C${endRow})`, `=SUMIF('Print Tracker'!I${startRow}:I${endRow},D6,'Print Tracker'!K${startRow}:K${endRow})`],
  [`=SUMIFS('Print Tracker'!C${startRow}:C${endRow},'Print Tracker'!I${startRow}:I${endRow},"")`, `=SUMIFS('Print Tracker'!K${startRow}:K${endRow},'Print Tracker'!I${startRow}:I${endRow},"")`],
];
summary.getRange("D3:F3").format = {
  fill: "#D9EAF7",
  font: { bold: true, color: "#1F2937" },
};
summary.getRange("D3:F7").format.borders = { preset: "all", style: "thin", color: "#D1D5DB" };
summary.getRange("E4:E7").format.numberFormat = "#,##0";
summary.getRange("F4:F7").format.numberFormat = "#,##0.0";

summary.getRange("A12:D21").values = [
  ["Material Type", "Total mg", "Total g", "Total kg"],
  ["PLA", null, null, null],
  ["PETG", null, null, null],
  ["ABS", null, null, null],
  ["ASA", null, null, null],
  ["TPU", null, null, null],
  ["Nylon", null, null, null],
  ["Other", null, null, null],
  ["", null, null, null],
  ["Grand total", null, null, null],
];
summary.getRange("B13:B19").formulas = [
  [`=SUMIF('Print Tracker'!D${startRow}:D${endRow},A13,'Print Tracker'!F${startRow}:F${endRow})`],
  [`=SUMIF('Print Tracker'!D${startRow}:D${endRow},A14,'Print Tracker'!F${startRow}:F${endRow})`],
  [`=SUMIF('Print Tracker'!D${startRow}:D${endRow},A15,'Print Tracker'!F${startRow}:F${endRow})`],
  [`=SUMIF('Print Tracker'!D${startRow}:D${endRow},A16,'Print Tracker'!F${startRow}:F${endRow})`],
  [`=SUMIF('Print Tracker'!D${startRow}:D${endRow},A17,'Print Tracker'!F${startRow}:F${endRow})`],
  [`=SUMIF('Print Tracker'!D${startRow}:D${endRow},A18,'Print Tracker'!F${startRow}:F${endRow})`],
  [`=SUMIF('Print Tracker'!D${startRow}:D${endRow},A19,'Print Tracker'!F${startRow}:F${endRow})`],
];
summary.getRange("C13").formulas = [["=B13/1000"]];
summary.getRange("C13:C19").fillDown();
summary.getRange("D13").formulas = [["=B13/1000000"]];
summary.getRange("D13:D19").fillDown();
summary.getRange("B21").formulas = [[`=SUM('Print Tracker'!F${startRow}:F${endRow})`]];
summary.getRange("C21").formulas = [["=B21/1000"]];
summary.getRange("D21").formulas = [["=B21/1000000"]];
summary.getRange("A12:D12").format = {
  fill: "#D9EAF7",
  font: { bold: true, color: "#1F2937" },
};
summary.getRange("A12:D21").format.borders = { preset: "all", style: "thin", color: "#D1D5DB" };
summary.getRange("B13:B21").format.numberFormat = "#,##0";
summary.getRange("C13:C21").format.numberFormat = "#,##0.0";
summary.getRange("D13:D21").format.numberFormat = "#,##0.000";
summary.getRange("A21:D21").format = { fill: "#EEF2F7", font: { bold: true } };

summary.getRange("F12:H21").values = [
  ["Material Color", "Total mg", "Total g"],
  ["Black", null, null],
  ["White", null, null],
  ["Matte White", null, null],
  ["Natural", null, null],
  ["Clear", null, null],
  ["Red", null, null],
  ["Blue", null, null],
  ["Other", null, null],
  ["Grand total", null, null],
];
summary.getRange("G13:G20").formulas = [
  [`=SUMIF('Print Tracker'!E${startRow}:E${endRow},F13,'Print Tracker'!F${startRow}:F${endRow})`],
  [`=SUMIF('Print Tracker'!E${startRow}:E${endRow},F14,'Print Tracker'!F${startRow}:F${endRow})`],
  [`=SUMIF('Print Tracker'!E${startRow}:E${endRow},F15,'Print Tracker'!F${startRow}:F${endRow})`],
  [`=SUMIF('Print Tracker'!E${startRow}:E${endRow},F16,'Print Tracker'!F${startRow}:F${endRow})`],
  [`=SUMIF('Print Tracker'!E${startRow}:E${endRow},F17,'Print Tracker'!F${startRow}:F${endRow})`],
  [`=SUMIF('Print Tracker'!E${startRow}:E${endRow},F18,'Print Tracker'!F${startRow}:F${endRow})`],
  [`=SUMIF('Print Tracker'!E${startRow}:E${endRow},F19,'Print Tracker'!F${startRow}:F${endRow})`],
  [`=SUMIF('Print Tracker'!E${startRow}:E${endRow},F20,'Print Tracker'!F${startRow}:F${endRow})`],
];
summary.getRange("H13").formulas = [["=G13/1000"]];
summary.getRange("H13:H20").fillDown();
summary.getRange("G21").formulas = [[`=SUM('Print Tracker'!F${startRow}:F${endRow})`]];
summary.getRange("H21").formulas = [["=G21/1000"]];
summary.getRange("F12:H12").format = {
  fill: "#D9EAF7",
  font: { bold: true, color: "#1F2937" },
};
summary.getRange("F12:H21").format.borders = { preset: "all", style: "thin", color: "#D1D5DB" };
summary.getRange("G13:G21").format.numberFormat = "#,##0";
summary.getRange("H13:H21").format.numberFormat = "#,##0.0";
summary.getRange("F21:H21").format = { fill: "#EEF2F7", font: { bold: true } };

for (const col of ["A", "B", "D", "E", "F", "G", "H"]) {
  summary.getRange(`${col}:${col}`).format.columnWidthPx = 120;
}
summary.getRange("C:C").format.columnWidthPx = 90;
summary.getRange("A:A").format.columnWidthPx = 160;
summary.getRange("D:D").format.columnWidthPx = 120;
summary.freezePanes.freezeRows(1);

lists.showGridLines = false;
lists.getRange("A1:C1").values = [["Material Type", "Priority", "Printer"]];
lists.getRange("A2:A8").values = [["PLA"], ["PETG"], ["ABS"], ["ASA"], ["TPU"], ["Nylon"], ["Other"]];
lists.getRange("B2:B4").values = [["High"], ["Medium"], ["Low"]];
lists.getRange("C2:C4").values = [["Steve"], ["Jimmy"], ["Adam"]];
lists.getRange("A1:C1").format = {
  fill: "#1F4E79",
  font: { bold: true, color: "#FFFFFF" },
};
lists.getRange("A:C").format.columnWidthPx = 140;
lists.getRange("A1:C8").format.borders = { preset: "all", style: "thin", color: "#D1D5DB" };

const checks = [];
checks.push(await workbook.inspect({
  kind: "table",
  range: "Print Tracker!A1:L8",
  include: "values,formulas",
  tableMaxRows: 8,
  tableMaxCols: 12,
  maxChars: 5000,
}));
checks.push(await workbook.inspect({
  kind: "table",
  range: "Summary!A1:H21",
  include: "values,formulas",
  tableMaxRows: 25,
  tableMaxCols: 8,
  maxChars: 5000,
}));
const errors = await workbook.inspect({
  kind: "match",
  searchTerm: "#REF!|#DIV/0!|#VALUE!|#NAME\\?|#N/A",
  options: { useRegex: true, maxResults: 300 },
  summary: "final formula error scan",
  maxChars: 4000,
});
console.log("INSPECT Print Tracker");
console.log(checks[0].ndjson);
console.log("INSPECT Summary");
console.log(checks[1].ndjson);
console.log("ERROR SCAN");
console.log(errors.ndjson);

for (const sheetName of ["Print Tracker", "Summary", "Lists"]) {
  const preview = await workbook.render({
    sheetName,
    autoCrop: "all",
    scale: 1,
    format: "png",
  });
  const bytes = new Uint8Array(await preview.arrayBuffer());
  await fs.writeFile(path.join(outputDir, `${sheetName.replaceAll(" ", "_").toLowerCase()}_preview.png`), bytes);
}

const output = await SpreadsheetFile.exportXlsx(workbook);
const outputPath = path.join(outputDir, "Tri Star Print Farm Tracker.xlsx");
await output.save(outputPath);
console.log(`SAVED ${outputPath}`);
