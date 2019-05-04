Development Requirements
------------------------
- node.js
- git
- python2?



Development Notes
-----------------
`node-gyp rebuild` needs to be run to compile the native code. This isn't directly available at the command line by default. So either `npm install` to indirectly trigger it to or `npm install -g node-gyp` to install it for direct use.\
https://nodejs.org/dist/latest/docs/api/addons.html#addons_building
https://github.com/nodejs/node-addon-api

I haven't taken the time to figure out how to debug native extensions directly.\
https://medium.com/@atulanand94/debugging-nodejs-c-addons-using-vs-code-27e9940fc3ad

Publishing an extension:
- npm install -g vsce
- vsce package
- code --install-extension path/to/ext.vsix

https://code.visualstudio.com/api/working-with-extensions/publishing-extension



Current State
-------------
Folding is in good working condition. Folding is determined entirely from tokens rather than from the AST. LibClang gives up building the AST when an unknown symbol is encountered (the symbol and its lexical children will be skipped and the parent scope will appear as a CompoundStmt). I haven't
tested LibTooling to see what it does because the current solution appears to be very nearly as good as using the AST anyway.

Blocks are folded using curly braces, block comments, and preprocessor blocks (#if and friends). Care is taken to fold the braces up onto the preceding line they are associated with (e.g. the function declaration, for keyword, etc). With closing braces it feels better to leave the brace exposed. This means most things fold into two lines: one with keyword/declaration and a trailing '...' and one with the closing brace '}'. I couldn't find compelling enough cases to justify folding things like function and template parameters (when spread across multiple lines). There might be room to do a token-AST hybrid approach (when the AST is actually available) where function calls that span multiple lines are folded, but that would introduce quite a lot of complexity and it just doesn't feel worth it.

Currently #defines do not fold. This should be reasonably straight forward, I just haven't gotten around to it.



To Do
-----
- [ ] Get LibTooling working and evaluate its capabilities
- [ ] Set folding range kind
- [ ] Consider folding #defines
- [ ] Ensure errors propagate somewhere visible
- [ ] Reduce the final vsix size
- [ ] Avoid copying file contents into a std::string (get the underlying v8 string)
- [ ] Avoid copying errors to an intermediate buffer
- [ ] Test performance
- [ ] Consider avoiding re-parsing when files have not changed
