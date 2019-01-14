import * as vscode from 'vscode';

export function activate(context: vscode.ExtensionContext) {
	// TODO: Do we need to dispose of anything?
	//context.subscriptions.push(disposable);

	var addon = require('bindings')('binding');

	vscode.languages.registerFoldingRangeProvider('cpp', {
		provideFoldingRanges(document, context, token) {
			var ranges = addon.ParseFile(document.fileName);
			//for (var i in ranges) {
			//	//console.log("[%i - %i]\n\tToken: %s (%s)\n\tCursor: %s (%s)",
			//	//	ranges[i].start + 1, ranges[i].end + 1,
			//	//	ranges[i].token, ranges[i].tokenKind,
			//	//	ranges[i].cursor, ranges[i].cursorKind,
			//	//);
			//	console.log("[%i - %i]", ranges[i].start + 1, ranges[i].end + 1);
			//}
			return ranges;
		}
	});
}

export function deactivate() {}
