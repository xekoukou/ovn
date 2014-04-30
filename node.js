//nodeSchema.js This is the schema of a node. This is a simple implementation that will have many performance problems
// this text file is for documentation purposes

module.exports = {

    node: {
        type: "object",
        properties: {
            id: {
                type: "integer"
            },
            parentId: {
                type: "integer"
            },
            input: {
                type: "array"
                items: {
                    type: "object",
                    "$ref": "#/definitions/link"
                }
            },
            output: {
                type: "array"
                items: {
                    type: "object",
                    "$ref": "#/definitions/link"
                }
            },
            nodeData: {
                type: "object",
                properties: {
                    summary: {
                        type: "String"
                    },
                    content: {
                        type: "String"
                    }
                },
                required: ["summary", "content"]
            },

        },
        required: ["id", "parentId", "input", "output", "nodeData"]
    },

    definitions: [

        link: {
            type: "object",
            properties: {
                id: {
                    type: "integer"
                },
                origId: {
                    type: "integer"
                },
                endId: {
                    type: "integer"
                },
                linkData: {
                    type: "object",
                    properties: {
                        summary: {
                            type: "String"
                        },
                        content: {
                            type: "String"
                        }
                    },
                    required: ["content", "summary"]
                }
            },
            required: ["id", "origId", "endId", "linkData"]
        }

    ]

}
