import * as vscode from 'vscode';

let addon: any;
let logWindow: vscode.OutputChannel;
let config: vscode.workspace.WorkspaceConfiguration;
let debug: boolean;

function log(...strs: any[]) {
	for (let s of strs) {
		logWindow.append(s.toString());
	}
	logWindow.appendLine('');
}

function debugLog(...strs: any[]) {
	if (debug) {
		for (let s of strs) {
			logWindow.append(s.toString());
		}
		logWindow.appendLine('');
	}
}

function onDidChangeConfiguration(event: vscode.Event<vscode.ConfigurationChangeEvent>) {
	log('onDidChangeConfiguration');
	config = vscode.workspace.getConfiguration();
	debug = config.get<boolean>('cppfold.debug');
	log('debug: ', debug);
}

function provideFoldingRanges(
	document: vscode.TextDocument,
	context: vscode.FoldingContext,
	token: vscode.CancellationToken
) {
	let result = addon.ParseFile(document.fileName, document.getText());

	// Errors
	let errors: string[] = result.errors;
	for (let error of errors) {
		log('Error: ' + error);
	}
	if (debug && errors.length > 0) {
		logWindow.show(true);
	}

	// Ranges
	let ranges: vscode.FoldingRange[] = result.ranges;
	debugLog(document.fileName + ' - ' + ranges.length + ' Ranges');
	return ranges;
}

export function activate(context: vscode.ExtensionContext) {
	logWindow = vscode.window.createOutputChannel('C++ Code Folding');
	log('activate');

	onDidChangeConfiguration(null);
	if (debug) {
		logWindow.show(true);
	}

	// TODO: Do we need to dispose of anything?
	//context.subscriptions.push(disposable);

	debugLog('Loading bindings...');
	addon = require('bindings')('binding');
	debugLog('Loaded bindings');

	vscode.workspace.onDidChangeConfiguration(onDidChangeConfiguration);
	vscode.languages.registerFoldingRangeProvider('cpp', { provideFoldingRanges });
}

//export function deactivate() {}
