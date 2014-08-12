module.exports = {

    request: {
        type: "object",
        properties: {
            signature: {
                type: "string"
            },
            publicKey: {
                type: "string"
            },
            data: {
                type: "object",
                properties: {
                    client_last_per_ordered_id: {
                        type: "integer"
                    },
                    client_last_per_set_id: {
                        type: "integer"
                    },
                    client_last_per_hist_id: {
                        type: "integer"
                    },
                    client_local_ordered_id: { /* arbitrary seyt by the client to order its own transactions when we have the same perceived state*/
                        type: "integer"
                    },
                    data: {
                        type: "object",
                        oneof: [{
                            "$ref": "#/definitions/recipeNewNode"
                        }]
                    }
                },
                required: ["client_last_per_ordered_id", "client_last_per_set_id", "client_last_per_hist_id", "client_local_ordered_id", "data"]
            }
        },
        required: ["signature", "publicKey", "data"],


        definitions: {
            recipeNewNode: {
                type: "object",
                properties: {
                    type: {
                        enum: ["recipeNewNode"]
                    },
                    ancestor_id: {
                        type: "integer"
                    },
                    ansestor_set_id: {
                        type: "integer"
                    },
                    previous_id: {
                        type: "integer"
                    },
                    previous_set_id: {
                        type: "integer"
                    },
                    data: {
                        type: "string"
                    }

                },
                required: ["ancestor_id", "ancestor_set_id", "previous_id", "previous_set_id", "data"]

            },

        }

    }
}
