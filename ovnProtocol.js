module.exports = {

    request: {
        type: "object",
        properties: {
            requestId: {
                type: "string"
            }
            request: {
                type: "object",
                oneof: [{
                    "$ref": "#/definitions/retrieveRequest"
                }]
            }

        },
        required: ["requestId", "request"]


    },

    response: {
        type: "object",
        properties: {
            requestId: {
                type: "string"
            }
            response: {
                type: "object",
                oneof: [{

                }]
            }


        },
        required: ["requestId", "response"]


    },

    definitions: [

        retrieveRequest: {
            type: "object",
            properties: {
                "type": {
                    "enum": ["retrieveRequest"]
                },

                idArray: {
                    type: "array",
                    minItems: 1,
                    items: {
                        type: "integer"
                    }


                },
                required: ["type", "idArray"]
            }
        },
        retrieveResponse: {

            type: "object",
            properties: {
                "type": {
                    "enum": ["retrieveResponse"]
                },

                nodeArray: {
                    type: "array",
                    minItems: 1,
                    items: {
                        type: "object",
                        "$ref": "node.js#/node"

                    }
                }
            },
            required: ["type", "nodeArray"]
        }

    ]


}
