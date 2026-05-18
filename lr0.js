let productions = [];
let prodCount = 0;
let symbols = [];
let symbolTypes = [];
let startSymbol = "";
let itemSets = [];
let itemSetCount = 0;

class Production {
    constructor(left, right) {
        this.left = left;
        this.right = right;
        this.rightLen = right.length;
    }
}

class Item {
    constructor(prodId, dotPos) {
        this.prodId = prodId;
        this.dotPos = dotPos;
    }
    
    equals(other) {
        return this.prodId === other.prodId && this.dotPos === other.dotPos;
    }
    
    toString() {
        let prod = productions[this.prodId];
        let result = prod.left + " -> ";
        for (let i = 0; i < prod.rightLen; i++) {
            if (i === this.dotPos) result += "·";
            result += prod.right[i];
            if (i < prod.rightLen - 1) result += " ";
        }
        if (this.dotPos === prod.rightLen) result += "·";
        return result;
    }
}

class ItemSet {
    constructor(id) {
        this.id = id;
        this.items = [];
        this.transitions = {};
    }
    
    addItem(item) {
        if (!this.hasItem(item)) {
            this.items.push(item);
            return true;
        }
        return false;
    }
    
    hasItem(item) {
        return this.items.some(i => i.equals(item));
    }
    
    equals(other) {
        if (this.items.length !== other.items.length) return false;
        for (let item of this.items) {
            if (!other.hasItem(item)) return false;
        }
        return true;
    }
}

function parseGrammar(grammarStr) {
    productions = [];
    prodCount = 0;
    const lines = grammarStr.split('\n');
    
    for (let line of lines) {
        line = line.trim();
        if (line === '' || line.startsWith('#')) continue;
        
        let arrowIndex = line.indexOf('->');
        if (arrowIndex === -1) continue;
        
        let left = line.substring(0, arrowIndex).trim();
        let rightPart = line.substring(arrowIndex + 2).trim();
        
        let rights = rightPart.split('|');
        for (let right of rights) {
            right = right.trim();
            let rightSymbols = right.split(/\s+/).filter(s => s !== '');
            productions.push(new Production(left, rightSymbols));
            prodCount++;
        }
    }
    
    if (productions.length > 0) {
        startSymbol = productions[0].left;
    }
    
    let augmentedLeft = startSymbol + "'";
    productions.push(new Production(augmentedLeft, [startSymbol]));
    prodCount++;
    
    buildSymbolTable();
    
    return { productions, prodCount, startSymbol, augmentedLeft };
}

function buildSymbolTable() {
    symbols = [];
    symbolTypes = [];
    
    for (let prod of productions) {
        if (!symbols.includes(prod.left)) {
            symbols.push(prod.left);
            symbolTypes.push('non-terminal');
        }
        for (let sym of prod.right) {
            if (!symbols.includes(sym)) {
                symbols.push(sym);
                if (isTerminal(sym)) {
                    symbolTypes.push('terminal');
                } else {
                    symbolTypes.push('non-terminal');
                }
            }
        }
    }
}

function isTerminal(symbol) {
    if (symbol.length === 1) {
        let c = symbol[0];
        if (c === '+' || c === '-' || c === '*' || c === '/' || 
            c === '(' || c === ')' || c === ';' || c === '=' ||
            c === '.' || c === ',') {
            return true;
        }
    }
    if (symbol === 'id') return true;
    return !(symbol[0] >= 'A' && symbol[0] <= 'Z');
}

function closure(itemSet) {
    let changed = true;
    while (changed) {
        changed = false;
        let itemsCopy = [...itemSet.items];
        
        for (let item of itemsCopy) {
            let prod = productions[item.prodId];
            if (item.dotPos < prod.rightLen) {
                let symbol = prod.right[item.dotPos];
                if (!isTerminal(symbol)) {
                    for (let i = 0; i < productions.length; i++) {
                        if (productions[i].left === symbol) {
                            let newItem = new Item(i, 0);
                            if (itemSet.addItem(newItem)) {
                                changed = true;
                            }
                        }
                    }
                }
            }
        }
    }
}

function gotoSet(itemSet, symbol) {
    let newSet = new ItemSet(-1);
    
    for (let item of itemSet.items) {
        let prod = productions[item.prodId];
        if (item.dotPos < prod.rightLen && prod.right[item.dotPos] === symbol) {
            newSet.addItem(new Item(item.prodId, item.dotPos + 1));
        }
    }
    
    if (newSet.items.length > 0) {
        closure(newSet);
    }
    
    return newSet;
}

function findItemSet(sets, target) {
    for (let i = 0; i < sets.length; i++) {
        if (sets[i].equals(target)) {
            return i;
        }
    }
    return -1;
}

function buildLR0Items() {
    itemSets = [];
    itemSetCount = 0;
    
    let I0 = new ItemSet(0);
    I0.addItem(new Item(prodCount - 1, 0));
    closure(I0);
    
    itemSets.push(I0);
    itemSetCount++;
    
    let changed = true;
    while (changed) {
        changed = false;
        
        for (let i = 0; i < itemSets.length; i++) {
            let current = itemSets[i];
            
            for (let sym of symbols) {
                let next = gotoSet(current, sym);
                if (next.items.length > 0) {
                    let existing = findItemSet(itemSets, next);
                    
                    if (existing === -1) {
                        next.id = itemSets.length;
                        itemSets.push(next);
                        existing = itemSets.length - 1;
                        changed = true;
                    }
                    
                    current.transitions[sym] = existing;
                }
            }
        }
    }
    
    itemSetCount = itemSets.length;
    return itemSets;
}

function checkConflicts() {
    let conflicts = [];
    
    for (let set of itemSets) {
        let reduceItems = [];
        let shiftItems = [];
        
        for (let item of set.items) {
            let prod = productions[item.prodId];
            if (item.dotPos === prod.rightLen) {
                reduceItems.push(item);
            } else if (item.dotPos < prod.rightLen) {
                let symbol = prod.right[item.dotPos];
                if (isTerminal(symbol)) {
                    shiftItems.push({ symbol, item });
                }
            }
        }
        
        if (reduceItems.length > 1) {
            conflicts.push({
                setId: set.id,
                type: 'reduce-reduce',
                count: reduceItems.length
            });
        }
        
        if (shiftItems.length > 0 && reduceItems.length > 0) {
            conflicts.push({
                setId: set.id,
                type: 'shift-reduce',
                shiftCount: shiftItems.length,
                reduceCount: reduceItems.length,
                shiftSymbols: shiftItems.map(s => s.symbol)
            });
        }
    }
    
    return conflicts;
}

function generateDot() {
    let dot = 'digraph LR0 {\n';
    dot += '    rankdir=LR;\n';
    dot += '    node [shape=circle];\n\n';
    
    for (let set of itemSets) {
        let isAccept = false;
        for (let item of set.items) {
            let prod = productions[item.prodId];
            if (item.dotPos === prod.rightLen && 
                prod.left === productions[prodCount-1].left) {
                isAccept = true;
                break;
            }
        }
        
        if (isAccept) {
            dot += `    I${set.id} [shape=doublecircle];\n`;
        }
        
        for (let [symbol, targetId] of Object.entries(set.transitions)) {
            dot += `    I${set.id} -> I${targetId} [label="${symbol}"];\n`;
        }
    }
    
    dot += '}\n';
    return dot;
}

function renderItemSets() {
    let html = '<div class="space-y-3">';
    
    for (let set of itemSets) {
        html += `<div class="item-set">`;
        html += `<div class="item-set-header">I${set.id} (${set.items.length}个项目)</div>`;
        html += `<div class="item-set-content">`;
        html += `<ul>`;
        for (let item of set.items) {
            html += `<li>${escapeHtml(item.toString())}</li>`;
        }
        html += `</ul>`;
        
        if (Object.keys(set.transitions).length > 0) {
            html += `<div class="transition-list">`;
            for (let [symbol, targetId] of Object.entries(set.transitions)) {
                html += `<span class="transition-item">${symbol} → I${targetId}</span>`;
            }
            html += `</div>`;
        }
        
        html += `</div></div>`;
    }
    
    html += '</div>';
    return html;
}

function renderConflicts(conflicts) {
    if (conflicts.length === 0) {
        return `<div class="conflict-success">✅ 无 LR(0) 冲突，该文法是 LR(0) 文法</div>`;
    }
    
    let html = `<div class="conflict-warning">❌ 发现 ${conflicts.length} 处冲突</div>`;
    for (let conflict of conflicts) {
        if (conflict.type === 'shift-reduce') {
            html += `<div class="conflict-warning mt-2">⚠️ I${conflict.setId}: 移进-归约冲突 - 遇到 ${conflict.shiftSymbols.join(', ')} 时，既可以移进也可以归约</div>`;
        } else if (conflict.type === 'reduce-reduce') {
            html += `<div class="conflict-warning mt-2">⚠️ I${conflict.setId}: 归约-归约冲突 - 有 ${conflict.count} 个归约项目</div>`;
        }
    }
    return html;
}

function renderGraph() {
    let nodes = [];
    let edges = [];
    
    for (let set of itemSets) {
        let isAccept = false;
        for (let item of set.items) {
            let prod = productions[item.prodId];
            if (item.dotPos === prod.rightLen && 
                prod.left === productions[prodCount-1].left) {
                isAccept = true;
                break;
            }
        }
        
        nodes.push({
            id: set.id,
            label: `I${set.id}`,
            shape: isAccept ? 'doublecircle' : 'circle',
            color: isAccept ? { background: '#a5f3c3', border: '#059669' } : { background: '#e0e7ff', border: '#6366f1' }
        });
        
        for (let [symbol, targetId] of Object.entries(set.transitions)) {
            edges.push({
                from: set.id,
                to: targetId,
                label: symbol,
                arrows: 'to',
                font: { size: 12 }
            });
        }
    }
    
    let container = document.getElementById('graph-container');
    let data = { nodes: new vis.DataSet(nodes), edges: new vis.DataSet(edges) };
    let options = {
        nodes: {
            shape: 'circle',
            size: 30,
            font: { size: 14, face: 'monospace' },
            borderWidth: 2
        },
        edges: {
            arrows: 'to',
            smooth: { type: 'curvedCW', roundness: 0.2 },
            font: { size: 11, align: 'middle' }
        },
        physics: { enabled: true, stabilization: { iterations: 100 } }
    };
    
    new vis.Network(container, data, options);
}

function escapeHtml(text) {
    let div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

function parseAndGenerate() {
    let grammarInput = document.getElementById('grammar-input').value;
    
    try {
        parseGrammar(grammarInput);
        buildLR0Items();
        let conflicts = checkConflicts();
        
        document.getElementById('item-sets-list').innerHTML = renderItemSets();
        document.getElementById('conflict-result').innerHTML = renderConflicts(conflicts);
        renderGraph();
        document.getElementById('result-area').classList.remove('hidden');
    } catch (error) {
        alert('解析错误：' + error.message);
    }
}

function handleFileUpload(file) {
    let reader = new FileReader();
    reader.onload = function(e) {
        document.getElementById('grammar-input').value = e.target.result;
        parseAndGenerate();
    };
    reader.readAsText(file);
}

document.addEventListener('DOMContentLoaded', function() {
    let parseBtn = document.getElementById('parse-grammar');
    if (parseBtn) {
        parseBtn.addEventListener('click', parseAndGenerate);
    }
    
    let fileUpload = document.getElementById('file-upload');
    if (fileUpload) {
        fileUpload.addEventListener('change', function(e) {
            if (e.target.files.length > 0) {
                handleFileUpload(e.target.files[0]);
            }
        });
    }
    
    parseAndGenerate();
});
