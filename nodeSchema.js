//nodeSchema.js This is the schema of a node. This is a simple implementation that will have many performance problems
// this text file is for documentation purposes

module.exports = {

    node: {
        type: "object",
        properties: {
            id: {
                type: "integer"
            },
            summary: {
                type: "String"
            },
            content: {
                type: "String"
            },
            input: {
                type: "array"
                items: {
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
            output: {
                type: "array"
                items: {
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
            }



        },
        required: ["id", "summary", "content", "input", "output"]

    },

    request: {
        type: "object",
        properties: {
            "type": {
                type: {
                    'enum': ["insert", "delete", "retrieve"]
                }
            },

            //when inserting
            "node": { //the node schema

	    	},

                //when deleting or rerieving
                "ids": {
                    "type": "array",
                    minItems: 1,
                    items: {
                        type: "Integer"
                    }
                }

            },
            required: ["type"]

        }

    }
