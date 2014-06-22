//nodeSchema.js This is the schema of a node. This is a simple implementation that will have many performance problems
// this text file is for documentation purposes

module.exports = {

    node: {
        type: "object",
        properties: {
            id: {
                type: "integer"
            },
            hid: {
                type: "string" //the sha2 of the previous historic identifier
            },
            ancestorId: {
                type: "integer"
            },
            ancestorHid: {
                type: "string"
            },
            parentId: {
                type: "integer"
            },
            input: {
                type: "array",
                items: {
                    type: "object",
                    "$ref": "#/definitions/link"
                }
            },
            output: {
                type: "array",
                items: {
                    type: "object",
                    "$ref": "#/definitions/link"
                }
            },
            nodeData: {
                type: "object",
                properties: {

                    id: {
                        type: "integer"
                    }
                    hid: {
                        type: "string"
                    }
                    summary: {
                        type: "string"
                    },
                    content: {}
                },
                required: ["id", "hid"]
            },

        },
        required: ["id", "hid", "parentId", "input", "output", "nodeData"],

        definitions: {

            link: {
                type: "object",
                properties: {
                    origId: {
                        type: "integer"
                    },
                    endId: {
                        type: "integer"
                    },
                    linkData: {
                        type: "object",
                        properties: {
                            //there may be many link with the same data per node.
                            //for example the same product tranfered to multiple destinations or the same varriable sent to many functions
                            //We need to show only one instance of that product varriable
                            id: {
                                type: "integer"
                            },
                            summary: {
                                type: "String"
                            },
                            content: {}
                        },
                        required: ["id", "summary"]
                    }
                },
                required: ["origId", "endId", "linkData"]
            }

        }
    }

}
