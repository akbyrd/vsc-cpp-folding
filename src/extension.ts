import * as vscode from 'vscode';

let log: vscode.OutputChannel;

export function activate(context: vscode.ExtensionContext) {
	// TODO: Dev only
	let debug: boolean = false;

	log = vscode.window.createOutputChannel('C++ Code Folding');
	log.show(debug);

	// TODO: Do we need to dispose of anything?
	//context.subscriptions.push(disposable);

	let addon = require('bindings')('binding');

	vscode.languages.registerFoldingRangeProvider('cpp', {
		provideFoldingRanges(
			document: vscode.TextDocument,
			context: vscode.FoldingContext,
			token: vscode.CancellationToken)
		{
			let result = addon.ParseFile(document.fileName, document.getText());

			// Errors
			let errors: string[] = result.errors;
			for (let error of errors) {
				log.appendLine('Error: ' + error);
			}
			//log.show(errors.length > 0);

			// Ranges
			let ranges: vscode.FoldingRange[] = result.ranges;
			log.appendLine(document.fileName + ' - ' + ranges.length + ' Ranges');
			return ranges;
		}
	});
}

export function deactivate() {}
